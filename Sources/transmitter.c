// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#include "transmitter.h"

#include "dmac.h"
#include "dfe.h"
#include "tone_generator.h"
#include "l1-trace.h"

#include "dma_common.h"
#include "iqstream_signals.h"

#include "axiq-la9310.h"
#include "vcpu.h"

#include "opstatus.h"
#include "vspa_iqstream.h"

#define PHY_TMR_DMA_CHAN 0
#define PHY_TMR_DMA_CHAN_MASK (1 << PHY_TMR_DMA_CHAN)

#define DMEM_ALIGNMENT_ATTR aligned(64)

#define TX_DAC_FIFO_BEAT_COUNT 16
#define dac_axi_fifo_addr (0x4400B000) // + 0x1000 - TX_DAC_FIFO_BEAT_COUNT * 16) // axi_DAC_FIFO_addr

#define XFER_SAMPLES 512

#define DAC_XFER_SAMPLE_COUNT XFER_SAMPLES
#define DAC_XFER_SIZE_BYTES (DAC_XFER_SAMPLE_COUNT * 4)

#define DDR_XFER_SAMPLE_COUNT XFER_SAMPLES
#define DDR_XFER_SIZE_BYTES (DDR_XFER_SAMPLE_COUNT * 4)

cfixed16_t dac_buffer[TX_MAX_LANE_COUNT][MAX_DMA_ENQ * DAC_XFER_SAMPLE_COUNT]
    __attribute__((DMEM_ALIGNMENT_ATTR, section(".vcpu_dmem")));
cfixed16_t ddr_read_buffer[TX_MAX_LANE_COUNT][MAX_DMA_ENQ * DDR_XFER_SAMPLE_COUNT]
    __attribute__((DMEM_ALIGNMENT_ATTR, section(".ippu_dmem")));

tone_state_t tx_generator[TX_MAX_LANE_COUNT];
dac_pipeline_t dac[TX_MAX_LANE_COUNT];

tx_ddr_pipeline_t txddr[TX_MAX_LANE_COUNT];
struct PipeStats tx_stats;

#define TX_MAX_UPSAMPLE_TAPS 64
cfixed16_t int_history[TX_MAX_UPSAMPLE_TAPS] __attribute__((aligned(64), section(".vcpu_dmem"))) = { 0 };
uint16_t int_ratio_pow2[TX_MAX_LANE_COUNT] = { 0 };

// Interpolation function prototypes
extern void X2_interp_tap32_filter(__fx16 *output, __fx16 *input, unsigned int num_samples, __fx16 *history, float *filter_taps);
extern void X4_interp_tap64_filter(__fx16 *output, __fx16 *input, unsigned int num_samples, __fx16 *history, float *filter_taps);

const float tx_filter_taps_upsampling_x2[32] __attribute__((aligned(64))) = {
#include "fir_interpolation_x2.txt"
};
// same coefficients as x2, just interleaved multiple instances.
const float tx_filter_taps_upsampling_x4[128] __attribute__((aligned(64))) = {
#include "fir_interpolation_x4.txt"
};

static inline void TxAXIQ(bool enabled) {
    if (enabled) {
        axiq_tx_enable();
        // log_info("TxAXIQ on" LOG_EOL);
    } else {
        axiq_tx_disable();
        // log_info("TxAXIQ off" LOG_EOL);
    }
}

static inline bool tx_axiq_is_enabled() {
    return gpord(7, (1 << 0)); // Tx AXIQ enable
}

static inline bool check_dac_had_issues() {
    // Check AXIQ tx fifo is not full or overrun
    uint32_t status = axiq_fifo_tx_sr(AXIQ_BANK_0, AXIQ_FIFO_TX0, AXIQ_SR_FIELD_ERROVER | AXIQ_SR_FIELD_ERRUNDER);
    if (status == 0)
        return false;

    const uint8_t field_shift = axiq_sr_shift(AXIQ_FIFO_TX0);
    status >>= field_shift;
    if (status & AXIQ_SR_FIELD_ERROVER) {
        ++tx_stats.afe_ovr;
        TRACE_COUNTER(CNT_TX_OVR, tx_stats.afe_ovr);
    }
    if (status & AXIQ_SR_FIELD_ERRUNDER) {
        ++tx_stats.afe_udr;
        TRACE_COUNTER(CNT_TX_UDR, tx_stats.afe_udr);
    }
    axiq_fifo_tx_cr(AXIQ_BANK_0, AXIQ_FIFO_TX0, AXIQ_CR_CLRERR, AXIQ_CR_CLRERR);
    axiq_fifo_tx_cr(AXIQ_BANK_0, AXIQ_FIFO_TX0, AXIQ_CR_CLRERR, 0);
    return true;
}

static inline void stream_write_ptr_rst_trig(uint16_t lane) {
    const uint32_t ctrl = DMAC_FIFO_RESET | DMAC_WRC | dac[lane].dma_channel;
    dmac_enable(ctrl, TX_DAC_FIFO_BEAT_COUNT * 16, dac[lane].axi_fifo_addr, VCPU_ADDR_FOR_DMA(dac_buffer[lane]));
}

static void tx_dac_reset(dac_pipeline_t *dac, cfixed16_t *buffer) {
    dac->base_buffer = buffer;
    dac->next_buffer = dac->base_buffer;
    dac->count_dmac_enque = 0;

    // reset dma
    const uint32_t dma_mask = (1 << dac->dma_channel);
    dmac_clear_complete(dma_mask);
    dmac_clear_event(dma_mask);
    dmac_clear_errxfr(dma_mask);
    dmac_clear_errcfg(dma_mask);
}

static void tx_ddr_reset(tx_ddr_pipeline_t *ddr, cfixed16_t *buffer) {
    ddr->base_buffer = buffer;
    ddr->enque_head = buffer;
    ddr->ready_buffer = buffer;
    ddr->count_dmac_enque = 0;
    ddr->count_dmac_complete = 0;
    ddr->ready_buffer_count = 0;
    ddr->ready_buffer_offset = 0;
    memclr(ddr->meta, sizeof(ddr->meta));

    ddr->dma_hif.tcd_table.done = 0;
    tcd_fifo_reset(&ddr->dma_hif.tcd_table);

    const uint32_t dma_mask = (1 << ddr->dma_channel);
    dmac_clear_complete(dma_mask);
    dmac_clear_event(dma_mask);
    dmac_clear_errxfr(dma_mask);
    dmac_clear_errcfg(dma_mask);
}

// Configure pipline's persistent parameters
void tx_lane_setup(uint16_t lane, uint16_t channel) {
    channel = 0; // TX has only 1 channel
    memclr(&tx_stats, sizeof(tx_stats));
    dac[lane].axi_fifo_addr = dac_axi_fifo_addr + (channel * 0x1000);
    dac[lane].axi_fifo_index = (enum axiq_fifo_e)(AXIQ_FIFO_TX0) + channel;
    dac[lane].dma_channel = TX_DAC_WR_DMA_CHANNEL + channel;
    tx_dac_reset(&dac[lane], dac_buffer[lane]);

    tx_generator[lane].amplitude = 0.9;
    tx_generator[lane].phase = 0;
    tx_generator[lane].freq_bin = 8192;

    txddr[lane].dma_channel = DDR_RD_DMA_CHANNEL_1; // + lane;
    tx_ddr_reset(&txddr[lane], ddr_read_buffer[lane]);

    memclr(int_history, sizeof(int_history));
}

void tx_lane_try_ddr_enqueue(tx_ddr_pipeline_t *ddr) {
    const uint16_t buffers_in_use = ddr->count_dmac_enque - ddr->count_dmac_complete;
    if (buffers_in_use >= MAX_DMA_ENQ) {
        return; // skip, all ddr buffers are in use
    }

    if (!dmac_is_available(1 << ddr->dma_channel)) {
        ++tx_stats.dfe_ovr;
        return;
    }

    if (tcd_fifo_isempty(&ddr->dma_hif.tcd_table)) {
        const uint32_t tx_dma_allowed = gpird(1, 1 << 16); // Phytimer trigger value
        if (tx_dma_allowed)
            ++tx_stats.dfe_udr;
        return;
    }

    TRACE_START_DURATION(t1);
    dma_tcd_t *tcd = tcd_fifo_front(&ddr->dma_hif.tcd_table);
    const uint32_t xfer_size = tcd->size > DDR_XFER_SIZE_BYTES ? DDR_XFER_SIZE_BYTES : tcd->size;

    iowr(DMA_DMEM_PRAM_ADDR, VCPU_ADDR_FOR_DMA(ddr->enque_head));
    iowr(DMA_AXI_ADDRESS, tcd->la9310_mem_address);
    iowr(DMA_AXI_BYTE_CNT, xfer_size);

    tcd->la9310_mem_address += xfer_size;
    tcd->size -= xfer_size;

    tx_meta_t *const meta = &ddr->meta[ddr->count_dmac_enque & 0x1];
    meta->flags = tcd->flags;

    if (tcd->size == 0) {
        meta->flags |= PKT_DMA_TCD_END;
        tcd_fifo_pop(&ddr->dma_hif.tcd_table);
    } else {
        meta->flags &= ~(PKT_IRQ | PKT_END); // not yet the end of TCD
    }

    const uint32_t dma_ctrl = DMAC_MBRE | DMAC_RDC | ddr->dma_channel | DMAC_TRIG_VCPU;
    iowr(DMA_XFR_CTRL, dma_ctrl); // ddr enque

    ++ddr->count_dmac_enque;
    ++tx_stats.dfe_enq;
    ddr->enque_head = ddr->base_buffer + (ddr->count_dmac_enque & 0x1) * DDR_XFER_SAMPLE_COUNT;
    TRACE_DURATION(T_DDR_WR, DEFAULT_THREAD_ID, t1);
}

static inline bool consume_ddr(uint16_t lane, tx_ddr_pipeline_t *ddr, uint16_t samplesCount) {
    ddr->ready_buffer_offset += samplesCount;
    if (ddr->ready_buffer_offset < DDR_XFER_SAMPLE_COUNT)
        return false;

    --ddr->ready_buffer_count;
    ddr->ready_buffer_offset = 0;

    ++ddr->count_dmac_complete;
    ddr->ready_buffer = ddr->base_buffer + (ddr->count_dmac_complete & 0x1) * DDR_XFER_SAMPLE_COUNT;
    return true;
}

inline static void dac_enque(dac_pipeline_t *dac, bool tx_burst_end) {
    uint32_t dma_ctrl = dac->dma_channel | DMAC_WRC | DMAC_FIFO | DMAC_TRIG_VCPU;
    if (tx_burst_end) {
        dma_ctrl |= DMAC_FIFO_RESET;
    }

    dmac_enable(dma_ctrl, // flags
                DAC_XFER_SIZE_BYTES, // size
                dac->axi_fifo_addr, // axi addr
                VCPU_ADDR_FOR_DMA(dac->next_buffer) // dmem addr
    );
    ++dac->count_dmac_enque;
    ++tx_stats.afe_enq;

    dac->next_buffer = dac->base_buffer + (dac->count_dmac_enque & 0x1) * DAC_XFER_SAMPLE_COUNT;
}

static inline void interpol(cfixed16_t *dest, cfixed16_t *src, cfixed16_t *history, uint16_t src_count) {
    uint16_t lane = 0;
    switch (int_ratio_pow2[lane]) {
    case 1:
        X2_interp_tap32_filter((__fx16 *)dest, (__fx16 *)src, src_count, (__fx16 *)int_history,
                               (float *)tx_filter_taps_upsampling_x2);
        src = dac[lane].next_buffer;
        break;
    case 2:
        X4_interp_tap64_filter((__fx16 *)dest, (__fx16 *)src, src_count, (__fx16 *)int_history,
                               (float *)tx_filter_taps_upsampling_x4);
        src = dac[lane].next_buffer;
        break;
    default:
        break;
    }
}

static inline void tx_pipeline_work(uint16_t lane) {
    if (txddr[lane].ready_buffer_count == 0)
        return;

    const uint16_t dma_mask = (1 << dac[lane].dma_channel);
    if (!dmac_is_available(dma_mask)) {
        return; // no free buffer. redundant check, should not happen
    }

    cfixed16_t *src = txddr[lane].ready_buffer + txddr[lane].ready_buffer_offset;
    cfixed16_t *dest = dac[lane].next_buffer;

    const uint16_t src_count = DAC_XFER_SAMPLE_COUNT >> int_ratio_pow2[lane];
    if (int_ratio_pow2[lane]) {
        interpol(dest, src, int_history, src_count);
        // qec inplace
        tx_qec_correction(dest, dest, DAC_XFER_SAMPLE_COUNT);
    } else {
        tx_qec_correction(dest, src, DAC_XFER_SAMPLE_COUNT);
    }

    tx_meta_t *const meta = &txddr[lane].meta[txddr[lane].count_dmac_complete & 0x1];
    // mark whole or part of available ddr data as consumed
    bool dac_end = consume_ddr(lane, &txddr[lane], src_count) && (meta->flags & PKT_END);

    check_dac_had_issues();

    dac_enque(dac, dac_end);
}

void dac_dma_complete(uint16_t lane) {
    TRACE_START_DURATION(t1);

    check_dac_had_issues();

    ++tx_stats.afe_compl;

    const uint32_t dma_mask = (1 << dac[lane].dma_channel);
    dmac_clear_complete(dma_mask);
    dmac_clear_event(dma_mask);

    // dac xfer has finished, freed up buffer for work output
    tx_pipeline_work(lane);

    // reenque ddr
    if (!tcd_fifo_isempty(&txddr[lane].dma_hif.tcd_table))
        tx_lane_try_ddr_enqueue(&txddr[lane]);

    TRACE_DURATION(T_AXIQ_COMPLETE, DEFAULT_THREAD_ID, t1);
}

void tx_ddr_complete(uint16_t lane) {
    TRACE_START_DURATION(t1);
    tx_ddr_pipeline_t *const ddr = &txddr[lane];
    const uint32_t dma_mask = (1 << ddr->dma_channel);
    dmac_clear_complete(dma_mask);
    dmac_clear_event(dma_mask);

    const uint16_t buf_index = ddr->count_dmac_complete + ddr->ready_buffer_count;
    tx_meta_t *const meta = &ddr->meta[buf_index & 0x1];

    if (meta->flags & PKT_DMA_TCD_END) {
        ++ddr->dma_hif.tcd_table.done;
        vspa_to_host_signal(ddr->dma_hif.vth_tcd_done_flag_mask);
    }

    ++tx_stats.dfe_compl;
    ++ddr->ready_buffer_count;

    tx_pipeline_work(lane);

    if (!tcd_fifo_isempty(&ddr->dma_hif.tcd_table))
        tx_lane_try_ddr_enqueue(ddr);
    TRACE_DURATION(T_DDR_WR_COMPLETE, DEFAULT_THREAD_ID, t1);
}

void transmitter_init(void) {
    TxAXIQ(false);
    for (int lane = 0; lane < TX_MAX_LANE_COUNT; ++lane) {
        vspa_dma_hif_t *dma_hif = &txddr[lane].dma_hif;
        dma_hif->tcd_table.done = 0;
        dma_hif->htv_tcd_pending_flag_mask = (HTV_SIGNAL_TXLANE0_TCD_PENDING << lane);
        dma_hif->vth_tcd_done_flag_mask = (VTH_SIGNAL_TXLANE0_TCD_DONE << lane);
        tcd_fifo_reset(&txddr[lane].tcd_table);
        clear_htv_signal(dma_hif->htv_tcd_pending_flag_mask);
    }

    tx_lane_setup(0, 0);
}

int tx_set_oversampling(uint16_t lane, uint16_t oversample_pow2) {
    if (lane > TX_MAX_LANE_COUNT)
        return lime_Result_InvalidValue;

    int_ratio_pow2[lane] = oversample_pow2;
    return lime_Result_Success;
}

static void inline tx_axiq_fifo_reset(uint16_t lane) {
    TxAXIQ(true); // enable just in case it wasn't. We'll need falling edge.
    const uint32_t dma_mask = 1 << dac[lane].dma_channel;

    // aborted DMA transactions won't trigger their complete/go/ptr_rst
    dmac_abort(dma_mask);
    TxAXIQ(false); // falling edge, enters DMA flush mode

    // ensure abort has ended before issuing new dma commands
    WAIT_FOR(!dmac_is_running(dma_mask), VSPA_DEFAULT_TIMEOUT);

    const uint32_t tx_dma_allowed = gpird(1, 1 << 16); // Phytimer trigger value
    if (tx_dma_allowed) // need dma allowed trigger for proper reset
    {
        stream_write_ptr_rst_trig(lane); // exit flush mode, tx_dma_allowed trigger must be still enabled at this point

        // wait for ptr reset
        WAIT_FOR(dmac_is_enabled(dma_mask), VSPA_DEFAULT_TIMEOUT);
    }

    dmac_clear_complete(dma_mask);
    dmac_clear_event(dma_mask);
}

// Resets pipeline and initiates start for new transmission
void tx_lane_prime(uint16_t lane) {
    tx_ddr_pipeline_t *const ddr = &txddr[lane];
    memclr(&tx_stats, sizeof(tx_stats));

    // reset ddr state
    tx_ddr_reset(ddr, ddr_read_buffer[lane]);

    // Prime dac AXIQ and DMA engine, the actual start is triggered by phytimer
    tx_dac_reset(&dac[lane], dac_buffer[lane]);

    tx_axiq_fifo_reset(lane);

    TxAXIQ(true);
    axiq_fifo_tx_cr(AXIQ_BANK_0, (enum axiq_fifo_e)dac->axi_fifo_index, AXIQ_CR_CLRERR, AXIQ_CR_CLRERR);
    axiq_fifo_tx_cr(AXIQ_BANK_0, (enum axiq_fifo_e)dac->axi_fifo_index, AXIQ_CR_CLRERR, 0);

    // if data available enque two buffers
    if (!tcd_fifo_isempty(&ddr->dma_hif.tcd_table)) {
        tx_lane_try_ddr_enqueue(ddr);
        tx_lane_try_ddr_enqueue(ddr);
    }
}

// Aborts any pending transfers and resets the pipeline's AXIQ FIFO
void tx_lane_abort(uint16_t lane) {
    const uint32_t dma_mask = (1 << dac[lane].dma_channel) | (1 << txddr[lane].dma_channel);

    // when DMA aborted, pending transactions won't trigger complete/go/ptr_rst
    dmac_abort(dma_mask);
    WAIT_FOR(!dmac_is_running(dma_mask), VSPA_DEFAULT_TIMEOUT);

    if (!tx_axiq_is_enabled())
        return; // axiq is already disabled, do nothing
    // tx_axiq_fifo_reset(lane);

    WAIT_FOR(!dmac_is_running(dma_mask), VSPA_DEFAULT_TIMEOUT);
    dmac_clear_complete(dma_mask);
    dmac_clear_event(dma_mask);
}
