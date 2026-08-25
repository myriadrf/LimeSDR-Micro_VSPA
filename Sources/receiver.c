// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#include "receiver.h"

#include "dmac.h"
#include "dfe.h"
#include "tone_generator.h"
#include "l1-trace.h"

#include "dma_common.h"
#include "iqstream_signals.h"

#include "axiq-la9310.h"
#include "vcpu.h"

#define DMEM_ALIGNMENT_ATTR aligned(64)

#define RO0_AXI_FIFO_ADDR 0x44001000
#define RO1_AXI_FIFO_ADDR 0x44002000
#define RX0_AXI_FIFO_ADDR 0x44003000
#define RX1_AXI_FIFO_ADDR 0x44004000

// #define RX_ADC_FIFO_BEAT_COUNT 16
// #define addr_beat_offset (0x1000 - RX_ADC_FIFO_BEAT_COUNT*16)
#define addr_beat_offset 0

#define MAX_DMA_ENQ 2
#define ADC_XFER_SAMPLE_COUNT 512
#define ADC_XFER_SIZE_BYTES (ADC_XFER_SAMPLE_COUNT * 4)

#define DDR_XFER_SAMPLE_COUNT 512
#define DDR_XFER_SIZE_BYTES (DDR_XFER_SAMPLE_COUNT * 4)

cfixed16_t adc_buffer[RX_MAX_LANE_COUNT][MAX_DMA_ENQ * ADC_XFER_SAMPLE_COUNT]
    __attribute__((DMEM_ALIGNMENT_ATTR, section(".ippu_dmem")));
cfixed16_t ddr_write_buffer[RX_MAX_LANE_COUNT][MAX_DMA_ENQ * DDR_XFER_SAMPLE_COUNT]
    __attribute__((DMEM_ALIGNMENT_ATTR, section(".ippu_dmem")));

// Decimation filter state
cfixed16_t decimation_history[RX_MAX_LANE_COUNT][32] __attribute__((aligned(64))) = { 0 };

// Filter coefficients, must be gain normalized
const float rx_filter_taps_downsampling[8] __attribute__((aligned(64))) = {
#include "fir_decimation_x2_x4.txt"
};

// ddc2x4x.sx prototypes
extern void decimator_2x_8_Taps_asm(cfixed16_t *pOut, cfixed16_t *pIn, float32_t *pTaps, cfixed16_t *filtState, uint32_t n_samples);
extern void decimator_4x_8_Taps_asm(cfixed16_t *pOut, cfixed16_t *pIn, float32_t *pTaps, cfixed16_t *filtState, uint32_t n_samples);

tone_state_t rx_generator[RX_MAX_LANE_COUNT];

struct DebugStats stats;

rx_ddr_pipeline_t rxddr[RX_MAX_LANE_COUNT];
adc_pipeline_t adc[RX_MAX_LANE_COUNT];

static inline void rx_adc_reset(adc_pipeline_t *adc, cfixed16_t *buffer) {
    adc->base_buffer = buffer;
    adc->next_completion_buffer = adc->base_buffer;
    adc->completion_count = 0;

    // reset dma
    const uint16_t dma_mask = (1 << adc->dma_channel);
    dmac_clear_complete(dma_mask);
    dmac_clear_event(dma_mask);
    dmac_clear_errxfr(dma_mask);
}

static inline void rx_ddr_reset(rx_ddr_pipeline_t *ddr, cfixed16_t *buffer) {
    ddr->base_buffer = buffer;
    ddr->write_head = ddr->base_buffer;
    ddr->count_enque = 0;
    ddr->buf_filled = 0;
    // tcd_fifo_reset(&ddr->tcd_fifo);
    const uint16_t dma_mask = (1 << ddr->dma_channel);
    dmac_clear_complete(dma_mask);
    dmac_clear_event(dma_mask);
    dmac_clear_errxfr(dma_mask);
}

void rx_setup_channel(uint16_t lane, e_rx_channel channel, uint16_t oversample_pow2) {
    adc[lane].axi_fifo_addr = RO0_AXI_FIFO_ADDR + (channel * 0x1000) + addr_beat_offset;
    adc[lane].axi_fifo_index = (enum axiq_fifo_e)(AXIQ_FIFO_RX0 + channel);
    adc[lane].dma_channel = RO0_ADC_RD_DMA_CHANNEL + channel;
    rx_adc_reset(&adc[lane], adc_buffer[lane]);

    rxddr[lane].dma_channel = DDR_WR_DMA_CHANNEL_1 + lane;
    rxddr[lane].decimate_pow2 = oversample_pow2;
    rx_ddr_reset(&rxddr[lane], ddr_write_buffer[lane]);

    rx_generator[lane].amplitude = 0.9;
    rx_generator[lane].phase = 0;
    rx_generator[lane].freq_bin = 8192;

    rxddr[lane].rx_host_if.tcd_done_counter = 0;
    rxddr[lane].rx_host_if.htv_pending_flag_mask = (HTV_SIGNAL_RXLANE0_TCD_PENDING << lane);
    rxddr[lane].rx_host_if.vth_tcd_done_flag_mask = (VTH_SIGNAL_RXLANE0_TCD_DONE << lane);

    clear_htv_signal(rxddr[lane].rx_host_if.htv_pending_flag_mask);
}

// Prime ADC AXIQ and DMA engine, the actual start is triggered by phytimer
static inline void adc_prime(uint16_t lane) {
    const dma_mask = 1 << adc[lane].dma_channel;
    if (dmac_is_enabled(dma_mask)) {
        // nothing should be scheduled at this point, something did not cleanup before.
        return;
    }

    // enque two reads
    const uint32_t dma_ctrl = adc[lane].dma_channel | DMAC_FIFO | DMAC_RDC | DMAC_TRIG_VCPU;
    dmac_prep_a_s(ADC_XFER_SIZE_BYTES, adc[lane].axi_fifo_addr);
    dmac_enable_v_c(dma_ctrl, adc_buffer[lane]);
    dmac_enable_v_c(dma_ctrl, &adc_buffer[lane][ADC_XFER_SAMPLE_COUNT]);
    stats.adc_enq += 2;

    // clear errors, enable axiq
    axiq_fifo_rx_enable(AXIQ_BANK_0, (enum axiq_fifo_e)adc[lane].axi_fifo_index);
    axiq_fifo_rx_cr(AXIQ_BANK_0, (enum axiq_fifo_e)adc[lane].axi_fifo_index, AXIQ_CR_CLRERR, AXIQ_CR_CLRERR);
    axiq_fifo_rx_cr(AXIQ_BANK_0, (enum axiq_fifo_e)adc[lane].axi_fifo_index, AXIQ_CR_CLRERR, 0);
}

inline static void rx_lane_try_ddr_enqueue(uint16_t lane) {
    TRACE_START_DURATION(t1);
    if (!dmac_is_available(1 << rxddr[lane].dma_channel)) {
        stats.ddr_err++;
        return;
    }

    if (tcd_fifo_isempty(&rxddr[lane].tcd_fifo)) {
        stats.ddr_ovr++;
        return;
    }

    vspa_dma_tcd_t *tcd = tcd_fifo_front(&rxddr[lane].tcd_fifo);
    const uint32_t xfer_size = tcd->size > DDR_XFER_SIZE_BYTES ? DDR_XFER_SIZE_BYTES : tcd->size;

    iowr(DMA_DMEM_PRAM_ADDR, VCPU_ADDR_FOR_DMA(rxddr[lane].write_head));
    iowr(DMA_AXI_ADDRESS, tcd->addr);
    iowr(DMA_AXI_BYTE_CNT, xfer_size);

    tcd->addr += xfer_size;
    tcd->size -= xfer_size;

    uint32_t dma_ctrl = DMAC_WRC | rxddr[lane].dma_channel; // no need DMAC_TRIG_VCPU, VCPU will be triggered by ADC transfer

    if (tcd->size == 0) {
        tcd_fifo_pop(&rxddr[lane].tcd_fifo);
        vspa_to_host_signal(rxddr[lane].rx_host_if.vth_tcd_done_flag_mask); // ask M4 to provide more TCD
        dma_ctrl |= DMAC_TRIG_IRQ; // signal M4 when data is actually available
    }

    iowr(DMA_XFR_CTRL, dma_ctrl);
    // TRACE_DMA_BEGIN(rxddr[lane].dma_channel, rxddr[lane].write_head);
    ++stats.ddr_enq;
    ++rxddr[lane].count_enque;
    rxddr[lane].write_head = rxddr[lane].base_buffer + (rxddr[lane].count_enque & 0x1) * DDR_XFER_SAMPLE_COUNT;
    TRACE_DURATION(T_DDR_WR, DEFAULT_THREAD_ID, t1);
}

static inline void decimate(uint16_t lane, cfixed16_t *dest, cfixed16_t *src, uint16_t src_count) {
    return;
    TRACE_START_DURATION(t1);
    switch (rxddr[lane].decimate_pow2) {
    case 1:
        decimator_2x_8_Taps_asm(dest, src, (float *)rx_filter_taps_downsampling, decimation_history[lane], src_count);
        break;
    case 2:
        decimator_4x_8_Taps_asm(dest, src, (float *)rx_filter_taps_downsampling, decimation_history[lane], src_count);
        break;
    default:
        break;
    }
    TRACE_DURATION(T_DEC_BUFFER, 1, t1);
}

void adc_dma_complete(uint16_t lane) {
    TRACE_START_DURATION(t1);
    TRACE_DMA_END(adc[lane].dma_channel, adc[lane].next_completion_buffer);

    const uint16_t dma_mask = (1 << adc[lane].dma_channel);
    dmac_clear_complete(dma_mask);
    dmac_clear_event(dma_mask);

    cfixed16_t *src = adc[lane].next_completion_buffer;
    cfixed16_t *dest = rxddr[lane].write_head + rxddr[lane].buf_filled;

    // work
    {
        TRACE_START_DURATION(t2);
        // in place processing
        // rx_qec_correction(adc[lane].next_completion_buffer, adc[lane].next_completion_buffer, ADC_XFER_SAMPLE_COUNT);
        // gen_nco_single_tone(rxddr[lane].write_head, ADC_XFER_SAMPLE_COUNT, &rx_generator[lane]);
        // gen_nco_single_tone(ddr_write_buffer[lane], 2*ADC_XFER_SAMPLE_COUNT, &rx_generator[lane]);
        rx_qec_correction(dest, src, ADC_XFER_SAMPLE_COUNT);
        TRACE_DURATION(T_QEC_RX_BUFFER, DEFAULT_THREAD_ID, t2);
    }

    const uint16_t input_count = ADC_XFER_SAMPLE_COUNT;

    if (rxddr[lane].decimate_pow2)
        decimate(lane, dest, src, ADC_XFER_SAMPLE_COUNT);

    rxddr[lane].buf_filled += (ADC_XFER_SAMPLE_COUNT >> rxddr[lane].decimate_pow2);

    // ADC self perpetuating, reenque new tranfer on each completion
    if (dmac_is_available(dma_mask)) {
        dmac_enable(adc[lane].dma_channel | DMAC_RDC | DMAC_FIFO | DMAC_TRIG_VCPU, // flags
                    ADC_XFER_SIZE_BYTES, // size
                    adc[lane].axi_fifo_addr, // axi addr
                    VCPU_ADDR_FOR_DMA(adc[lane].next_completion_buffer) // dmem addr
        );
        stats.adc_enq++;
    } else
        stats.adc_err++;

    if (rxddr[lane].buf_filled >= DDR_XFER_SAMPLE_COUNT) {
        rx_lane_try_ddr_enqueue(lane);
        rxddr[lane].buf_filled = 0;
    }

    ++stats.adc_compl;
    ++adc[lane].completion_count;
    adc[lane].next_completion_buffer = adc[lane].base_buffer + (adc[lane].completion_count & 0x1) * ADC_XFER_SAMPLE_COUNT;

    TRACE_DURATION(T_ADC_COMPLETE, DEFAULT_THREAD_ID, t1);
}

void ddr_dma_complete(uint16_t lane) {
    TRACE_START_DURATION(t1);
    // TRACE_DMA_END(rxddr[lane].dma_channel, adc[lane].next_completion_buffer);
    const uint32_t dma_mask = (1 << rxddr[lane].dma_channel);
    dmac_clear_complete(dma_mask);
    // dmac_clear_event(dma_mask); // go event not used for ddr

    ++stats.ddr_compl;
    ++rxddr[lane].rx_host_if.tcd_done_counter;

    TRACE_DURATION(T_DDR_WR_COMPLETE, DEFAULT_THREAD_ID, t1);
}

void receiver_init(void) {
    for (int i = 0; i < RX_MAX_LANE_COUNT; ++i)
        rx_setup_channel(i, VSPA_RX0 + i, 0);
}

void rx_lane_prime(uint16_t lane) {
    rx_adc_reset(&adc[lane], adc_buffer[lane]);
    rx_ddr_reset(&rxddr[lane], ddr_write_buffer[lane]);

    memclr(&stats, sizeof(stats));
    adc_prime(lane);
}

// PTR_RST must be done after:
// dma_allowed falling edge
static void stream_read_ptr_rst(uint16_t lane) {
    // const uint32_t ctrl = DMAC_PEND_EXT | DMAC_FIFO_RESET | DMAC_RDC | adc[lane].dma_channel;
    const uint32_t ctrl = DMAC_FIFO_RESET | DMAC_RDC | adc[lane].dma_channel;
    dmac_enable(ctrl, 128, adc[lane].axi_fifo_addr, VCPU_ADDR_FOR_DMA(adc_buffer[lane]));
}

void rx_lane_stop(uint16_t lane) {
    // const uint32_t rx_dma_allowed = gpird(0, (1<< adc[lane].axi_fifo_index * 4)); // Phytimer trigger value

    const uint32_t dma_mask = (1 << rxddr[lane].dma_channel) | (1 << adc[lane].dma_channel);
    dmac_abort(dma_mask);
    axiq_fifo_rx_disable(AXIQ_BANK_0, (enum axiq_fifo_e)adc[lane].axi_fifo_index); // enter DMA flush mode

    tcd_fifo_reset(&rxddr[lane].tcd_fifo);
    while (!dmac_is_available(1 << adc[lane].dma_channel)) {
    }
    stream_read_ptr_rst(lane); // exits flush mode, tx_dma_allowed trigger must be still enabled at this point
    while (dmac_is_running(1 << adc[lane].dma_channel)) // wait for abort to complete
    {
    }
    dmac_clear_complete(dma_mask);
    dmac_clear_event(dma_mask);
}

bool rx_insert_tcd(uint16_t lane, const vspa_dma_tcd_t *tcd) {
    if (tcd_fifo_isfull(&rxddr[lane].tcd_fifo))
        return false;

    // limit to external memory range
    if (tcd->addr < 0xA0000000 || (tcd->addr + tcd->size) > 0xDFFFFFFF)
        return false;

    tcd_fifo_push(&rxddr[lane].tcd_fifo, *tcd);
    return true;
}
