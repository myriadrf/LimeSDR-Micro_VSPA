// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#include "receiver.h"

#include "dmac.h"
#include "dfe.h"
#include "tone_generator.h"
#include "l1-trace.h"

#include "dma_common.h"
#include "iqstream_signals.h"
#include "vspa_iqstream.h"

#include "axiq-la9310.h"
#include "vcpu.h"

#include "opstatus.h"

#define DMEM_ALIGNMENT_ATTR aligned(64)

#define RO0_AXI_FIFO_ADDR 0x44001000
#define RO1_AXI_FIFO_ADDR 0x44002000
#define RX0_AXI_FIFO_ADDR 0x44003000
#define RX1_AXI_FIFO_ADDR 0x44004000

// #define RX_ADC_FIFO_BEAT_COUNT 16
// #define addr_beat_offset (0x1000 - RX_ADC_FIFO_BEAT_COUNT*16)
#define addr_beat_offset 0

#define XFER_SAMPLES 512

#define ADC_XFER_SAMPLE_COUNT XFER_SAMPLES
#define ADC_XFER_SIZE_BYTES (ADC_XFER_SAMPLE_COUNT * 4)

#define DDR_XFER_SAMPLE_COUNT XFER_SAMPLES
#define DDR_XFER_SIZE_BYTES (DDR_XFER_SAMPLE_COUNT * 4)

cfixed16_t adc_buffer[RX_MAX_LANE_COUNT][MAX_DMA_ENQ * ADC_XFER_SAMPLE_COUNT]
    __attribute__((DMEM_ALIGNMENT_ATTR, section(".vcpu_dmem")));
cfixed16_t ddr_write_buffer[RX_MAX_LANE_COUNT][MAX_DMA_ENQ * DDR_XFER_SAMPLE_COUNT]
    __attribute__((DMEM_ALIGNMENT_ATTR, section(".ippu_dmem")));

// Decimation filter state
cfixed16_t decimation_history[RX_MAX_LANE_COUNT][32] __attribute__((aligned(64))) = { 0 };

// Filter coefficients, must be gain normalized
const float rx_filter_taps_downsampling[8] __attribute__((aligned(64))) = {
#include "fir_decimation_x2_x4.txt"
};

// ddc2x4x.sx prototypes
extern void decimator_2x_8_Taps_asm(cfixed16_t *pOut, volatile cfixed16_t *pIn, const float32_t *pTaps, cfixed16_t *filtState,
                                    uint32_t n_samples);
extern void decimator_4x_8_Taps_asm(cfixed16_t *pOut, volatile cfixed16_t *pIn, const float32_t *pTaps, cfixed16_t *filtState,
                                    uint32_t n_samples);

tone_state_t rx_generator[RX_MAX_LANE_COUNT];

struct PipeStats rx_stats[RX_MAX_LANE_COUNT];

rx_ddr_pipeline_t rxddr[RX_MAX_LANE_COUNT];
adc_pipeline_t adc[RX_MAX_LANE_COUNT];

static inline void check_adc_axi_status(uint16_t lane) {
    const enum axiq_fifo_e fifo_index = (enum axiq_fifo_e)adc[lane].axi_fifo_index;
    const uint8_t field_shift = axiq_sr_shift(fifo_index);

    // Check AXIQ rx fifo is not full or overrun
    uint32_t status = axiq_fifo_rx_sr(AXIQ_BANK_0, fifo_index, AXIQ_SR_FIELD_ERROVER | AXIQ_SR_FIELD_ERRUNDER);
    if (status == 0)
        return;

    status >>= field_shift;
    if (status & AXIQ_SR_FIELD_ERROVER) {
        ++rx_stats[lane].afe_ovr;
    }
    if (status & AXIQ_SR_FIELD_ERRUNDER) {
        ++rx_stats[lane].afe_udr;
    }
    axiq_fifo_rx_cr(AXIQ_BANK_0, fifo_index, AXIQ_CR_CLRERR, AXIQ_CR_CLRERR);
    axiq_fifo_rx_cr(AXIQ_BANK_0, fifo_index, AXIQ_CR_CLRERR, 0);
}

// PTR_RST must be done for each dma_allowed falling edge
static inline void stream_read_ptr_rst(uint16_t lane) {
    const uint32_t ctrl = DMAC_FIFO_RESET | DMAC_RDC | adc[lane].dma_channel;
    dmac_enable(ctrl, 16, adc[lane].axi_fifo_addr, VCPU_ADDR_FOR_DMA(adc_buffer[lane]));
}

static void rx_axiq_fifo_reset(uint16_t lane) {
    axiq_fifo_rx_enable(AXIQ_BANK_0, (enum axiq_fifo_e)adc[lane].axi_fifo_index);
    const uint32_t dma_mask = (1 << adc[lane].dma_channel);
    dmac_abort(dma_mask);

    axiq_fifo_rx_disable(AXIQ_BANK_0, (enum axiq_fifo_e)adc[lane].axi_fifo_index); // falling edge enters flush mode
    WAIT_FOR(!dmac_is_running(dma_mask), VSPA_DEFAULT_TIMEOUT);
    stream_read_ptr_rst(lane); // exits flush mode

    WAIT_FOR(!dmac_is_enabled(dma_mask), VSPA_DEFAULT_TIMEOUT);
    dma_clear_all_bits(dma_mask);
}

static inline void rx_adc_pipe_reset(adc_pipeline_t *adc, cfixed16_t *buffer) {
    adc->base_buffer = buffer;
    adc->next_completion_buffer = buffer;
    adc->count_dmac_complete = 0;

    // reset dma
    dma_clear_all_bits(1 << adc->dma_channel);
}

static inline void rx_ddr_pipe_reset(rx_ddr_pipeline_t *ddr, cfixed16_t *buffer) {
    ddr->base_buffer = buffer;
    ddr->write_head = buffer;
    ddr->count_dmac_enque = 0;
    ddr->count_dmac_complete = 0;
    ddr->buf_filled = 0;
    tcd_fifo_reset(&ddr->dma.tcd_table);
    memclr(ddr->meta, sizeof(ddr->meta));

    const uint32_t dma_mask = (1 << ddr->dma_channel);
    dmac_abort(dma_mask);
    WAIT_FOR(!dmac_is_running(dma_mask), VSPA_DEFAULT_TIMEOUT);
    dma_clear_all_bits(dma_mask);
}

int rx_select_channel(uint16_t lane, e_rx_channel channel) {
    if (lane > RX_MAX_LANE_COUNT)
        return lime_Result_InvalidValue;
    adc[lane].axi_fifo_addr = RO0_AXI_FIFO_ADDR + (channel * 0x1000) + addr_beat_offset;
    adc[lane].axi_fifo_index = (enum axiq_fifo_e)(AXIQ_FIFO_RX0 + channel);
    adc[lane].dma_channel = RO0_ADC_RD_DMA_CHANNEL + channel;
    rx_adc_pipe_reset(&adc[lane], adc_buffer[lane]);

    rxddr[lane].dma_channel = DDR_WR_DMA_CHANNEL_1 + lane;
    rx_ddr_pipe_reset(&rxddr[lane], ddr_write_buffer[lane]);

    rx_generator[lane].amplitude = 0.9;
    rx_generator[lane].phase = 0;
    rx_generator[lane].freq_bin = 8192;

    rxddr[lane].dma.htv_tcd_pending_flag_mask = (HTV_SIGNAL_RXLANE0_TCD_PENDING << lane);
    rxddr[lane].dma.vth_tcd_done_flag_mask = (VTH_SIGNAL_RXLANE0_TCD_DONE << lane);

    clear_htv_signal(rxddr[lane].dma.htv_tcd_pending_flag_mask);
    return lime_Result_Success;
}

int rx_set_oversampling(uint16_t lane, uint16_t decimate_pow2) {
    if (lane > RX_MAX_LANE_COUNT)
        return lime_Result_InvalidValue;

    rxddr[lane].decimate_pow2 = decimate_pow2;
    return lime_Result_Success;
}

// Prime ADC AXIQ and DMA engine, the actual start is triggered by phytimer
static inline void initial_adc_enq(uint16_t lane) {
    const uint16_t dma_mask = 1 << adc[lane].dma_channel;

    WAIT_FOR(!dmac_is_running(1 << adc[lane].dma_channel), VSPA_DEFAULT_TIMEOUT);
    axiq_fifo_rx_enable(AXIQ_BANK_0, (enum axiq_fifo_e)adc[lane].axi_fifo_index);
    // Enabling Rx AXIQ instantly generates underrun error, clear it.
    axiq_fifo_rx_cr(AXIQ_BANK_0, (enum axiq_fifo_e)adc[lane].axi_fifo_index, AXIQ_CR_CLRERR, AXIQ_CR_CLRERR);
    axiq_fifo_rx_cr(AXIQ_BANK_0, (enum axiq_fifo_e)adc[lane].axi_fifo_index, AXIQ_CR_CLRERR, 0);

    // enque two reads
    const uint32_t dma_ctrl = adc[lane].dma_channel | DMAC_FIFO | DMAC_RDC | DMAC_TRIG_VCPU;
    dmac_prep_a_s(ADC_XFER_SIZE_BYTES, adc[lane].axi_fifo_addr);
    dmac_enable_v_c(dma_ctrl, VCPU_ADDR_FOR_DMA(adc_buffer[lane]));
    dmac_enable_v_c(dma_ctrl, VCPU_ADDR_FOR_DMA(&adc_buffer[lane][ADC_XFER_SAMPLE_COUNT]));
    rx_stats[lane].afe_enq += 2;
}

inline static void rx_lane_try_ddr_enqueue(uint16_t lane) {
    TRACE_START_DURATION(t1);
    rx_ddr_pipeline_t *const ddr = &rxddr[lane];
    if (!dmac_is_available(1 << ddr->dma_channel)) {
        ++rx_stats[lane].dfe_ovr;
        return;
    }

    if (tcd_fifo_isempty(&ddr->dma.tcd_table)) {
        ++rx_stats[lane].dfe_udr;
        return;
    }

    rx_meta_t *const meta = &ddr->meta[ddr->count_dmac_enque & 0x1];
    meta->flags = 0;

    volatile dma_tcd_t *const tcd = tcd_fifo_front(&ddr->dma.tcd_table);
    const uint32_t xfer_size = tcd->size > DDR_XFER_SIZE_BYTES ? DDR_XFER_SIZE_BYTES : tcd->size;

    iowr(DMA_DMEM_PRAM_ADDR, VCPU_ADDR_FOR_DMA(ddr->write_head));
    iowr(DMA_AXI_ADDRESS, tcd->la9310_mem_address);
    iowr(DMA_AXI_BYTE_CNT, xfer_size);

    tcd->la9310_mem_address += xfer_size;
    tcd->size -= xfer_size;

    // DDR writes complete faster than ADC reads, so no need for DMAC_TRIG_VCPU
    // VCPU will be triggered only by ADC transfers to maintain consistent pacing
    uint32_t dma_ctrl = DMAC_WRC | ddr->dma_channel;
    if (tcd->size == 0) {
        meta->flags |= PKT_DMA_TCD_END;
        tcd_fifo_pop(&ddr->dma.tcd_table);
    }

    iowr(DMA_XFR_CTRL, dma_ctrl);
    // TRACE_DMA_BEGIN(ddr->dma_channel, ddr->write_head);
    ++rx_stats[lane].dfe_enq;
    ++ddr->count_dmac_enque;
    ddr->write_head = ddr->base_buffer + (ddr->count_dmac_enque & 0x1) * DDR_XFER_SAMPLE_COUNT;
    TRACE_DURATION(T_DDR_WR, DEFAULT_THREAD_ID, t1);
}

static inline void decimate(uint16_t lane, cfixed16_t *restrict dest, volatile cfixed16_t *restrict src, uint16_t src_count) {
    TRACE_START_DURATION(t1);
    switch (rxddr[lane].decimate_pow2) {
    default:
    case 1:
        decimator_2x_8_Taps_asm(dest, src, rx_filter_taps_downsampling, decimation_history[lane], src_count);
        break;
    case 2:
        decimator_4x_8_Taps_asm(dest, src, rx_filter_taps_downsampling, decimation_history[lane], src_count);
        break;
    }
    TRACE_DURATION(T_DEC_BUFFER, 1, t1);
}

static const float downscalefactor = 0.25f;

void adc_dma_complete(uint16_t lane) {
    TRACE_START_DURATION(t1);
    TRACE_DMA_END(adc[lane].dma_channel, adc[lane].next_completion_buffer);
    cfixed16_t *const completed_buffer = adc[lane].next_completion_buffer;

    check_adc_axi_status(lane);

    const uint16_t dma_mask = (1 << adc[lane].dma_channel);
    dmac_clear_complete(dma_mask);
    dmac_clear_event(dma_mask);

    ++rx_stats[lane].afe_compl;

    rx_ddr_pipeline_t *const ddr = &rxddr[lane];
    cfixed16_t *const dest = ddr->write_head + ddr->buf_filled;

    // work
    // downscale into 12bit range
    rhf_rhf_rsp_vMultiSclr_asm(completed_buffer, completed_buffer, &downscalefactor, ADC_XFER_SIZE_BYTES/DMEM_LINE_SIZE_BYTES);

    const uint16_t input_count = ADC_XFER_SAMPLE_COUNT;
    if (ddr->decimate_pow2) {
        TRACE_START_DURATION(t2);
        // in place processing
        rx_qec_correction(completed_buffer, completed_buffer, ADC_XFER_SAMPLE_COUNT);
        TRACE_DURATION(T_QEC_RX_BUFFER, DEFAULT_THREAD_ID, t2);
        decimate(lane, dest, completed_buffer, ADC_XFER_SAMPLE_COUNT);
    } else {
        TRACE_START_DURATION(t2);
        // in place processing
        // rx_qec_correction(adc[lane].next_completion_buffer, adc[lane].next_completion_buffer, ADC_XFER_SAMPLE_COUNT);
        // gen_nco_single_tone(rxddr[lane].write_head, ADC_XFER_SAMPLE_COUNT, &rx_generator[lane]);
        // gen_nco_single_tone(ddr_write_buffer[lane], 2*ADC_XFER_SAMPLE_COUNT, &rx_generator[lane]);

        // process into ddr buffer
        rx_qec_correction(dest, completed_buffer, ADC_XFER_SAMPLE_COUNT);
        TRACE_DURATION(T_QEC_RX_BUFFER, DEFAULT_THREAD_ID, t2);
    }

    ddr->buf_filled += (input_count >> ddr->decimate_pow2);

    // ADC self perpetuating, reenque new tranfer on each completion
    if (dmac_is_available(dma_mask)) {
        dmac_enable(adc[lane].dma_channel | DMAC_RDC | DMAC_FIFO | DMAC_TRIG_VCPU, // flags
                    ADC_XFER_SIZE_BYTES, // size
                    adc[lane].axi_fifo_addr, // axi addr
                    VCPU_ADDR_FOR_DMA(completed_buffer) // dmem addr
        );
        ++rx_stats[lane].afe_enq;
    } else
        ++rx_stats[lane].afe_ovr;

    if (ddr->buf_filled >= DDR_XFER_SAMPLE_COUNT) {
        rx_lane_try_ddr_enqueue(lane);
        ddr->buf_filled = 0;
    }

    ++adc[lane].count_dmac_complete;
    adc[lane].next_completion_buffer = adc[lane].base_buffer + (adc[lane].count_dmac_complete & 0x1) * ADC_XFER_SAMPLE_COUNT;

    TRACE_DURATION(T_ADC_COMPLETE, DEFAULT_THREAD_ID, t1);
}

void ddr_dma_complete(uint16_t lane) {
    TRACE_START_DURATION(t1);
    // TRACE_DMA_END(rxddr[lane].dma_channel, adc[lane].next_completion_buffer);
    rx_ddr_pipeline_t *const ddr = &rxddr[lane];
    const rx_meta_t *const meta = &ddr->meta[ddr->count_dmac_complete & 0x1];

    ++rx_stats[lane].dfe_compl;
    ++ddr->count_dmac_complete;
    dmac_clear_complete(1 << ddr->dma_channel);
    // dmac_clear_event(1 << ddr->dma_channel); // go event not used for ddr

    if (meta->flags & PKT_DMA_TCD_END) {
        ++ddr->dma.tcd_table.done;
        vspa_to_host_signal(ddr->dma.vth_tcd_done_flag_mask);
    }

    TRACE_DURATION(T_DDR_WR_COMPLETE, DEFAULT_THREAD_ID, t1);
}

void receiver_init(void) {
    for (int i = 0; i < RX_MAX_LANE_COUNT; ++i) {
        rx_select_channel(i, (e_rx_channel)(VSPA_RX0 + i));
        rx_set_oversampling(i, 0);
    }
}

void rx_lane_prime(uint16_t lane) {
    rx_axiq_fifo_reset(lane);
    rx_adc_pipe_reset(&adc[lane], adc_buffer[lane]);
    rx_ddr_pipe_reset(&rxddr[lane], ddr_write_buffer[lane]);

    memclr(&rx_stats[lane], sizeof(struct PipeStats));
    initial_adc_enq(lane);
}

void rx_lane_stop(uint16_t lane) {
    const uint32_t dma_mask = (1 << rxddr[lane].dma_channel) | (1 << adc[lane].dma_channel);
    dmac_abort(dma_mask);
    // axiq_fifo_rx_disable(AXIQ_BANK_0, (enum axiq_fifo_e)adc[lane].axi_fifo_index); // enter DMA flush mode
    // WAIT_FOR(dmac_is_available(1 << adc[lane].dma_channel), VSPA_DEFAULT_TIMEOUT);
    // stream_read_ptr_rst(lane);
    // WAIT_FOR(!dmac_is_enabled(dma_mask), VSPA_DEFAULT_TIMEOUT);

    dmac_clear_complete(dma_mask);
    dmac_clear_event(dma_mask);
}
