// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#include "transmitter.h"

#include "dmac.h"
#include "dfe.h"
#include "tone_generator.h"
#include "l1-trace.h"

#include "dma_common.h"
#include "iqstream_signals.h"

#include "opstatus.h"
#include "vspa_iqstream.h"
#include "phytimer.h"
#include "proxy_data.h"

#define PHY_TMR_DMA_CHAN 0
#define PHY_TMR_DMA_CHAN_MASK (1 << PHY_TMR_DMA_CHAN)

#define TX_DAC_FIFO_BEAT_COUNT 16
#define dac_axi_fifo_addr (0x4400B000) // + 0x1000 - TX_DAC_FIFO_BEAT_COUNT * 16) // axi_DAC_FIFO_addr

#define DAC_XFER_SAMPLE_COUNT AXIQ_XFER_SAMPLES_COUNT
#define DAC_XFER_SIZE_BYTES (DAC_XFER_SAMPLE_COUNT * 4)
#define DAC_DMEM_LINES_COUNT (DAC_XFER_SIZE_BYTES / 128)

#define DDR_XFER_SAMPLE_COUNT AXIQ_XFER_SAMPLES_COUNT
#define DDR_XFER_SIZE_BYTES (DDR_XFER_SAMPLE_COUNT * 4)

extern uint32_t stream_origin_phyticks;
float upscalefactor[TX_MAX_LANE_COUNT]; // when user sample format is only 12bit full scale, upscale to 16bits full scale

static const uint32_t reschedule_time = DAC_XFER_SAMPLE_COUNT / 2;
// Order: DAC DMA allowed, PA_EN, RF switch. The RF switch comparator (RFCTL_0) is written last so that it can be left
// out: on LimeSDR-Micro v1+ it is the Tx band switch (TX_SW), which LimeSDR_Micro::SetAntenna sets with a forced output.
// Re-programming it, even with "no change", disables it first, and that drops the forced output to 0.
static const uint32_t tx_timer_addrs[9] = {
    // DAC DMA
    PHY_TMR_SC_ADDR(PHY_TIMER_COMP_CH5_TX_ALLOWED), PHY_TMR_SC_ADDR(PHY_TIMER_COMP_CH5_TX_ALLOWED),
    PHY_TMR_V_ADDR(PHY_TIMER_COMP_CH5_TX_ALLOWED),
    // PA_EN
    PHY_TMR_SC_ADDR(PHY_TIMER_COMP_RFCTL_5), PHY_TMR_SC_ADDR(PHY_TIMER_COMP_RFCTL_5), PHY_TMR_V_ADDR(PHY_TIMER_COMP_RFCTL_5),
    // RF switch
    PHY_TMR_SC_ADDR(PHY_TIMER_COMP_RFCTL_0), PHY_TMR_SC_ADDR(PHY_TIMER_COMP_RFCTL_0), PHY_TMR_V_ADDR(PHY_TIMER_COMP_RFCTL_0)
};
static uint32_t tx_timer_values[9];

// in VCPU DMEM: the IPPU DMEM holds the ADC buffers and the DDR read ring
cfixed16_t dac_buffer[TX_MAX_LANE_COUNT][MAX_DMA_ENQ * DAC_XFER_SAMPLE_COUNT] _VSPA_VECTOR_ALIGN;
cfixed16_t ddr_read_buffer[TX_MAX_LANE_COUNT][TX_DDR_RING * DDR_XFER_SAMPLE_COUNT] _VSPA_VECTOR_ALIGN
    __attribute__((section(".bss.ippu")));

tx_pipeline_t txpipe[TX_MAX_LANE_COUNT] = { { false } };
tone_state_t tx_tone_state[TX_MAX_LANE_COUNT];
dac_pipeline_t tx_dac[TX_MAX_LANE_COUNT];

tx_ddr_pipeline_t txddr[TX_MAX_LANE_COUNT];
struct PipeStats tx_stats;
tx_tdd_config_t tx_tdd;

// Transmission window end diagnostics (host-readable in DMEM)
struct TxWindowStats {
    uint32_t go1_ext; // windows closed by the GO_1 phytimer event
    uint32_t go1_fallback; // windows closed by the phytime check because no GO_1 event came
    uint32_t end_late; // END was already in the past when it was scheduled (the compare can never match)
    uint32_t phytmr_busy; // a raw phytimer DMA found the channel still busy after the timeout
} tx_window_stats;

// The DAC start/end/stop schedules write the comparators with a raw DMA on PHY_TMR_DMA_CHAN, the channel the
// timer_trig_*() helpers use as well. dmac_enable() on a channel that is still running is lost, so wait for it.
static inline void phytmr_dma_wait_free(void) {
    WAIT_FOR(!dmac_is_enabled(PHY_TMR_DMA_CHAN_MASK), VSPA_DEFAULT_TIMEOUT);
    if (dmac_is_enabled(PHY_TMR_DMA_CHAN_MASK))
        ++tx_window_stats.phytmr_busy;
}

// when DAC clock divider is disabled, 1 PHYtimer tick can represent only >=2 samples period
static const uint16_t dac_clock_divider_disabled = 0; // TODO: read from config registers

#define TX_MAX_UPSAMPLE_TAPS 64
cfixed16_t int_history[TX_MAX_UPSAMPLE_TAPS] __attribute__((aligned(64), section(".bss.vcpu")));
uint16_t int_ratio_pow2[TX_MAX_LANE_COUNT] = { 0 }; // copy of the lane's channel config, latched on lane prime

static tx_channel_config_t tx_channel_cfg[TX_CHANNEL_COUNT];

static inline tx_meta_t *tx_ddr_next_enq_meta(tx_ddr_pipeline_t *tx) { return &tx->meta[tx->enq_slot]; }
static inline tx_meta_t *tx_ddr_next_completed_meta(tx_ddr_pipeline_t *tx) { return &tx->meta[tx->cpl_slot]; }
static inline cfixed16_t *tx_ddr_current_ready_buffer(const tx_ddr_pipeline_t *tx) {
    return tx->ddr.base_buffer + tx->consumed_slot * DDR_XFER_SAMPLE_COUNT;
}

static tx_meta_t *tx_ddr_current_ready_meta(tx_ddr_pipeline_t *tx) { return &tx->meta[tx->consumed_slot]; }

static inline bool is_tx_dma_allowed(void) { return (gpird(1, 1 << 16)); }

static inline uint32_t ts_to_phytime(uint32_t ts, uint16_t oversample_pow2) {
    return (ts << oversample_pow2) >> dac_clock_divider_disabled;
}

static inline cfixed16_t *tx_ddr_next_enq_buffer(const tx_ddr_pipeline_t *tx) {
    // TODO: use modulo buffer registers for circular addressing?
    return tx->ddr.base_buffer + tx->enq_slot * DDR_XFER_SAMPLE_COUNT;
}

static inline tx_meta_t *dac_next_enq_meta(dac_pipeline_t *pipe) { return &pipe->meta[pipe->afe.count_dmac_enque & 0x1]; }

static inline tx_meta_t *dac_next_free_meta(dac_pipeline_t *pipe) { return &pipe->meta[pipe->afe.buffers_prepared & 0x1]; }

static inline tx_meta_t *dac_next_complete_meta(dac_pipeline_t *pipe) { return &pipe->meta[pipe->afe.count_dmac_complete & 0x1]; }

static inline void schedule_dac_start(const tx_tdd_config_t *tdd, uint32_t phytime) {
    phytmr_dma_wait_free();
    tx_timer_values[0] = PHY_TIMER_COMPARATOR_DISABLE | PHY_TIMER_COMPARATOR_CLEAR_INT;
    tx_timer_values[1] = ePhyTimerComparatorOut1;
    tx_timer_values[2] = phytime + tdd->dac_allowed;
    tx_timer_values[3] = PHY_TIMER_COMPARATOR_DISABLE | PHY_TIMER_COMPARATOR_CLEAR_INT;
    tx_timer_values[4] = tdd->pa_on_comparator_out;
    tx_timer_values[5] = phytime + tdd->pa_on;
    tx_timer_values[6] = PHY_TIMER_COMPARATOR_DISABLE | PHY_TIMER_COMPARATOR_CLEAR_INT;
    tx_timer_values[7] = tdd->rf_sw_on_comparator_out;
    tx_timer_values[8] = phytime + tdd->rf_sw_on;
    const uint16_t count = tdd->rf_sw_on_comparator_out == ePhyTimerComparatorNoChange ? 6 : 9;
    dmac_enable(DMAC_DI32 | PHY_TMR_DMA_CHAN, count, VSPA_HALF_WORDS(tx_timer_addrs), VSPA_HALF_WORDS(tx_timer_values));
}

// DAC AXIQ transfers have minimum size requirements, therefore it's disable phytime might be past the amount
// of RF samples that were provided. The RF switch toggle phytime should be at the actual end of RF samples
// to cut off padded samples.
static inline void schedule_dac_end(const tx_tdd_config_t *tdd, uint32_t phytime, uint32_t rf_switch_phytime) {
    phytmr_dma_wait_free();
    tx_timer_values[0] = PHY_TIMER_COMPARATOR_DISABLE | PHY_TIMER_COMPARATOR_CLEAR_INT;
    tx_timer_values[1] = ePhyTimerComparatorOut0;
    tx_timer_values[2] = phytime + tdd->dac_allowed;
    tx_timer_values[3] = PHY_TIMER_COMPARATOR_DISABLE | PHY_TIMER_COMPARATOR_CLEAR_INT;
    tx_timer_values[4] = tdd->pa_off_comparator_out;
    tx_timer_values[5] = phytime + tdd->pa_off;
    tx_timer_values[6] = PHY_TIMER_COMPARATOR_DISABLE | PHY_TIMER_COMPARATOR_CLEAR_INT;
    tx_timer_values[7] = tdd->rf_sw_off_comparator_out;
    tx_timer_values[8] = rf_switch_phytime + tdd->rf_sw_off;
    const uint16_t count = tdd->rf_sw_off_comparator_out == ePhyTimerComparatorNoChange ? 6 : 9;
    dmac_enable(DMAC_DI32 | PHY_TMR_DMA_CHAN, count, VSPA_HALF_WORDS(tx_timer_addrs), VSPA_HALF_WORDS(tx_timer_values));
}

// Interpolation function prototypes
extern void X2_interp_tap32_filter(__fx16 *output, __fx16 *input, unsigned int num_samples, __fx16 *history, float *filter_taps);
extern void X4_interp_tap64_filter(__fx16 *output, __fx16 *input, unsigned int num_samples, __fx16 *history, float *filter_taps);

// TODO: reduce memory use, coefficients are identical, only values duplicated for x4
const float tx_filter_taps_upsampling_x2[32] _VSPA_VECTOR_ALIGN = {
#include "fir_interpolation_x2.txt"
};
// same coefficients as x2, just interleaved multiple instances.
const float tx_filter_taps_upsampling_x4[128] _VSPA_VECTOR_ALIGN = {
#include "fir_interpolation_x4.txt"
};

static void dac_stop_immediate() {
    phytmr_dma_wait_free();
    tx_timer_values[0] = PHY_TIMER_COMPARATOR_DISABLE | PHY_TIMER_COMPARATOR_CLEAR_INT;
    tx_timer_values[1] = ePhyTimerComparatorOut0 << 2;
    dmac_enable(DMAC_DI32 | PHY_TMR_DMA_CHAN, 2, VSPA_HALF_WORDS(tx_timer_addrs), VSPA_HALF_WORDS(tx_timer_values));
}

// @brief Attempt to enque all available DDR buffers (free ring slots, at most MAX_DMA_ENQ reads in flight)
static void tx_initial_ddr_enque(tx_ddr_pipeline_t *tx) {
    ddr_pipeline_t *const ddr = &tx->ddr;
    for (int i = 0; i < TX_DDR_RING; ++i) {
        const uint16_t in_use = ddr_tx_buffers_in_use(ddr);
        if (in_use >= TX_DDR_RING || ddr_xfer_in_flight(ddr) >= MAX_DMA_ENQ)
            break;
        tx_lane_try_ddr_enqueue(tx, true);
        if (ddr_tx_buffers_in_use(ddr) == in_use)
            break; // nothing queued by the host
    }
}

static inline void TxAXIQ(bool enabled) {
    if (enabled) {
        axiq_tx_enable();
    } else {
        axiq_tx_disable();
    }
}

static inline bool tx_axiq_is_enabled() {
    return gpord(7, (1 << 0)); // Tx AXIQ enable
}

static void tx_dac_reset(dac_pipeline_t *dac, cfixed16_t *buffer) {
    afe_pipe_reset(&dac->afe, buffer); // also clears the DMA channel's status bits

    dac->expected_end_phytime = 0;
    dac->in_progress = false;
    dac->end_scheduled = false;
    dac->end_deferred = false;
    dac->force_start = false;
    memset16(dac->meta, 0, sizeof(dac->meta));
}

static void tx_host_dma_reset(tx_ddr_pipeline_t *tx) {
    tx->ddr.dma.tcd_table.done = 0;
    tcd_fifo_reset(&tx->ddr.dma.tcd_table);
    proxy.tx_dma_done = 0;
    proxy_dirty = true;
}

static void tx_ddr_reset(tx_ddr_pipeline_t *tx, cfixed16_t *buffer) {
    timer_trig_immediate(PHY_TIMER_COMP_VSPA_GO_1, ePhyTimerComparatorOut0);
    ddr_pipe_reset(&tx->ddr, buffer);
    tx->enq_slot = tx->cpl_slot = tx->consumed_slot = 0;
    tx->ready_buffer_offset = 0;
    memset16(tx->meta, 0, sizeof(tx->meta));

    const uint32_t dma_mask = (1 << tx->ddr.dma_channel);
    dmac_abort(dma_mask);
    WAIT_FOR(!dmac_is_running(dma_mask), VSPA_DEFAULT_TIMEOUT);

    dma_clear_all_bits(dma_mask);
}

// Configure pipline's persistent parameters
void tx_lane_setup(uint16_t lane, uint16_t channel) {
    channel = 0; // TX has only 1 channel
    memset16(&tx_stats, 0, sizeof(tx_stats));
    tx_dac[lane].channel = channel;

    tx_axiq_fill_config(&tx_dac[lane].afe.config, channel);
    tx_dac_reset(&tx_dac[lane], dac_buffer[lane]);

    tx_tone_state[lane].amplitude = 0.9;
    tx_tone_state[lane].phase = 0;
    tx_tone_state[lane].fcw = 0x7FFFFFFFu >> 2; // FS/8

    txddr[lane].ddr.dma_channel = DDR_RD_DMA_CHANNEL_1; // + lane;
    tx_ddr_reset(&txddr[lane], ddr_read_buffer[lane]);
    tx_host_dma_reset(&txddr[lane]);

    memset16(int_history, 0, sizeof(int_history));
}

void tx_lane_try_ddr_enqueue(tx_ddr_pipeline_t *tx, bool vcpu_trig) {
    TRACE_START_DURATION(t1);
    ddr_pipeline_t *const ddr = &tx->ddr;

    if (txpipe[0].generate_tone)
        return;
    // ring full, or the DMA channel already has its maximum of reads in flight
    if (ddr_tx_buffers_in_use(ddr) >= TX_DDR_RING || ddr_xfer_in_flight(ddr) >= MAX_DMA_ENQ)
        return;

    const uint32_t tcd_count = tcd_fifo_size(&ddr->dma.tcd_table);
    TRACE_COUNTER(CNT_TX_TCD, tcd_count);
    if (tcd_count == 0) {
        if (is_tx_dma_allowed()) {
            // Tx transmission is active right now, and host has not provided more data
            ++tx_stats.host_udr;
        }
        return;
    }

    dma_tcd_t *tcd;
    tx_meta_t *ddr_meta;
    do {
        tcd = tcd_fifo_front(&ddr->dma.tcd_table);
        // TODO: drop tcds that are already late
        ddr_meta = tx_ddr_next_enq_meta(tx);

        // phytimer stream start offset will be applied right before scheduling.
        ddr_meta->phytime = ts_to_phytime(tcd->timestamp_lsb, int_ratio_pow2[0]);

        TRACE_COUNTER(CNT_DDR_ENQ_ADVANCE, (ddr_meta->phytime + stream_origin_phyticks) - phytime_now());
        break;
    } while (1);

    cfixed16_t *const dest = tx_ddr_next_enq_buffer(tx);
    uint32_t xfer_size = DDR_XFER_SIZE_BYTES;
    if (tcd->size < DDR_XFER_SIZE_BYTES) {
        xfer_size = tcd->size;
        // host can send any amount of data, but internally it's processed in fixed size chunks.
        // clear residual data if host is not going to overwrite it all.
        memset16(dest, 0, DDR_XFER_SAMPLE_COUNT * sizeof(cfixed16_t));
    }

    iowr(DMA_DMEM_PRAM_ADDR, VCPU_ADDR_FOR_DMA(dest));
    iowr(DMA_AXI_ADDRESS, tcd->la9310_mem_address);
    // Even though host can submit xfer size less than the buffer size
    // read the whole buffer's worth of data, to maintain DMA xfers tracking.
    // If two buffers are enqueued, first one large + second small, then both might
    // get completed under the same dma_complete bit trigger, throwing off event counting.
    iowr(DMA_AXI_BYTE_CNT, DDR_XFER_SIZE_BYTES);

    tcd->la9310_mem_address += xfer_size;
    tcd->size -= xfer_size;
    tcd->timestamp_lsb += DDR_XFER_SAMPLE_COUNT;

    ddr_meta->flags = tcd->flags;
    tcd->flags &= ~(IQSTREAM_TRX_START); // clear start tag once used

    if (tcd->size == 0) {
        ddr_meta->flags |= IQSTREAM_DMA_TCD_END;
        tcd_fifo_pop(&ddr->dma.tcd_table);
    } else {
        ddr_meta->flags &= ~(IQSTREAM_IRQ | IQSTREAM_TRX_END); // not yet the end of packet
    }

    // It seems that when PLATFORM_CLOCK is <30MHz, DMAC_MBRE can potentially result in
    // DMA getting stuck in running state and never complete. Exact reasons unknown, and
    // the clock limit is rough observation not guarantee.
    // uint32_t dma_ctrl = DMAC_MBRE | DMAC_RDC | ddr->dma_channel;
    uint32_t dma_ctrl = DMAC_RDC | ddr->dma_channel;
    // Multi-burst reads (2101c9f) are off: on lp1 at 30.72 MS/s without interpolation (oversampling 1) they made the
    // DDR read of a burst's last transfer complete late (DAC starved at every few bursts' tail), left the read channel
    // stuck after a session (the next session never got a DDR completion), and cut the OAI UE's DL TCP from 9.2-10.1
    // to 0.24-3.9 Mbit/s. TX_DDR_MBRE=1 restores them for clocks where they work.
#if defined(TX_DDR_MBRE) && TX_DDR_MBRE
    if (int_ratio_pow2[0] == 0)
        dma_ctrl |= DMAC_MBRE; // if interpolation is not used, we're running at >40Msps, use burst mode.
#endif
    if (vcpu_trig)
        dma_ctrl |= DMAC_TRIG_VCPU;
    iowr(DMA_XFR_CTRL, dma_ctrl);
    TRACE_DMA_BEGIN(ddr->dma_channel, ddr->count_dmac_enque & 1);

    ++ddr->count_dmac_enque;
    tx->enq_slot = tx_ddr_next_slot(tx->enq_slot);
    ++tx_stats.dfe_enq;
    TRACE_COUNTER(CNT_TX_DDR_ENQ, ddr->count_dmac_enque - ddr->count_dmac_complete);
    TRACE_DURATION(T_DDR_RD_ENQ, DEFAULT_THREAD_ID, t1);
}

inline static void dac_enque(dac_pipeline_t *dac, cfixed16_t *buffer, uint32_t extra_dmac_flags) {
    TRACE_START_DURATION(t1);
    afe_pipeline_t *const afe = &dac->afe;
    if (!dmac_is_available(1 << afe_dma_channel(afe))) {
        ++tx_stats.afe_err;
    }
    // when tx_dma_allowed is not yet triggered, DAC will immediately start to output
    // the first sample value of enqueued buffer.It will appear as DC until DMA is triggered.
    dmac_enable(afe->config.dmac_ctrl_flags | DMAC_PEND_EXT | DMAC_TRIG_VCPU | extra_dmac_flags, DAC_XFER_SIZE_BYTES,
                afe->config.axi_fifo_addr, VCPU_ADDR_FOR_DMA(buffer));
    TRACE_DMA_BEGIN(afe_dma_channel(afe), afe->count_dmac_enque & 1);

    ++afe->count_dmac_enque;
    ++tx_stats.afe_enq;
    TRACE_DURATION(T_DAC_ENQ, DEFAULT_THREAD_ID, t1);
}

static inline void interpol(cfixed16_t *dest, cfixed16_t *src, cfixed16_t *history, uint16_t src_count) {
    uint16_t lane = 0;
    switch (int_ratio_pow2[lane]) {
    case 1:
        X2_interp_tap32_filter((__fx16 *)dest, (__fx16 *)src, src_count, (__fx16 *)int_history,
                               (float *)tx_filter_taps_upsampling_x2);
        break;
    case 2:
        X4_interp_tap64_filter((__fx16 *)dest, (__fx16 *)src, src_count, (__fx16 *)int_history,
                               (float *)tx_filter_taps_upsampling_x4);
        break;
    default:
        break;
    }
}

// Schedule the end of the current transmission window at the end of this DAC buffer.
static inline void dac_schedule_end(dac_pipeline_t *pipe, const tx_meta_t *dac_meta) {
    uint32_t dac_off_phytime;
    if (dac_meta->flags & IQSTREAM_HAS_TIMESTAMP)
        dac_off_phytime = dac_meta->phytime + stream_origin_phyticks + DAC_XFER_SAMPLE_COUNT;
    else
        dac_off_phytime = pipe->expected_end_phytime + DAC_XFER_SAMPLE_COUNT;
    if ((int32_t)(dac_off_phytime - phytime_now()) <= 0) {
        // The END compare can never match a value that already passed: Tx allowed would stay high, no GO_1 would
        // come and the next START would have no rising edge. Stop now and let the window close.
        ++tx_window_stats.end_late;
        dac_stop_immediate();
    } else
        schedule_dac_end(&tx_tdd, dac_off_phytime, dac_off_phytime);
    pipe->end_scheduled = true;

    // current transmission window is active, and phytimers might be scheduled to trigger it's end.
    // Cannot schedule next window start, while the timers are still pending and
    // last DAC buffer DMA completion event triggers some time before the actual end of transmission,
    // so it cannot be used as phytimer rescheduling point. Need to schedule a dedicated GO event which
    // would trigger right after the actual transmission end and phytimers are done.
    pipe->end_go_phytime = dac_off_phytime + DAC_XFER_SAMPLE_COUNT;
    // GO_1 fires on a rising edge only: make sure the previous window's Out1 was cleared
    timer_trig_immediate(PHY_TIMER_COMP_VSPA_GO_1, ePhyTimerComparatorOut0);
    timer_trig_schedule(PHY_TIMER_COMP_VSPA_GO_1, ePhyTimerComparatorOut1, pipe->end_go_phytime);
}

static inline void dac_enq_pending_buffer(dac_pipeline_t *pipe) {
    tx_meta_t *const dac_meta = dac_next_enq_meta(pipe);
    afe_pipeline_t *const afe = &pipe->afe;
    if (dac_meta->flags & IQSTREAM_TRX_START || pipe->force_start) {
        if (pipe->in_progress)
            return; // can't schedule new start while previous transmission did not reach end.
        uint32_t tx_start_phytime;
        if (dac_meta->flags & IQSTREAM_HAS_TIMESTAMP) {
            tx_start_phytime = dac_meta->phytime + stream_origin_phyticks;
            const uint32_t tsAdvance = tx_start_phytime - phytime_now();
            TRACE_COUNTER(CNT_DAC_PHYTIME_ADVANCE, tsAdvance);
            if (tsAdvance < reschedule_time || tsAdvance > 0x7FFFFFFFu) {
                ++tx_stats.afe_drop;
                --afe->buffers_prepared;
                // The dropped buffer carried the burst's START. Without it the following buffers of the burst would be
                // enqueued with no DAC start scheduled and wait for ever (Tx stall). Start the burst at its first buffer
                // that is still in time instead, as the underrun recovery does.
                pipe->force_start = true;
                return;
            }
        } else // transmit immediately
            tx_start_phytime = phytime_now() + reschedule_time;

        schedule_dac_start(&tx_tdd, tx_start_phytime);
        TRACE_EVENT(T_TX_BURST_START, 3, dac_meta->phytime);
        pipe->in_progress = true;
        pipe->start_phytime = tx_start_phytime;
        pipe->force_start = false;
        pipe->expected_end_phytime = tx_start_phytime + DAC_XFER_SAMPLE_COUNT;
        // A burst that fits in one DAC buffer carries START and END together, and it needs its END too: otherwise no
        // GO_1 closes the window and every later START waits for ever. START and END use the same comparator (Tx
        // allowed), so scheduling the END now would overwrite the pending START: schedule it when this buffer's DAC
        // transfer completes, which is after the START fired.
        if (dac_meta->flags & IQSTREAM_TRX_END)
            pipe->end_deferred = true;
    } else if (dac_meta->flags & IQSTREAM_TRX_END) {
        // Short burst (two DAC buffers, or one DDR buffer interpolated): the END buffer is enqueued before the START
        // fired. Same comparator: defer the END as above.
        if (pipe->in_progress && (int32_t)(phytime_now() - pipe->start_phytime) < 0)
            pipe->end_deferred = true;
        else
            dac_schedule_end(pipe, dac_meta);
    } else
        pipe->expected_end_phytime += DAC_XFER_SAMPLE_COUNT;
    const uint32_t dac_dma_flags = dac_meta->flags & IQSTREAM_TRX_END ? DMAC_FIFO_RESET : 0;
    dac_enque(pipe, afe_next_enq_buffer(afe), dac_dma_flags);
}

static void ddr_to_dac_work(uint16_t lane) {
    TRACE_START_DURATION(t1);
    tx_ddr_pipeline_t *const tx = &txddr[lane];
    dac_pipeline_t *const dac = &tx_dac[lane];
    afe_pipeline_t *const axiq = &tx_dac[lane].afe;

    cfixed16_t *const ddr_src = tx_ddr_current_ready_buffer(tx);
    tx_meta_t *const ddr_meta = tx_ddr_current_ready_meta(tx);

    cfixed16_t *const dac_dest = afe_dac_next_free_buffer(axiq);
    tx_meta_t *const dac_meta = dac_next_free_meta(dac);
    dac_meta->flags = ddr_meta->flags;
    dac_meta->phytime = ddr_meta->phytime;

    if (int_ratio_pow2[lane]) {
        if (ddr_meta->flags & IQSTREAM_TRX_START) {
            ddr_meta->flags ^= IQSTREAM_TRX_START; // clear as ddr buffer is consumed over multiple interpolations
            if (txpipe[lane].generate_tone)
                gen_nco_single_tone(ddr_src, DDR_XFER_SIZE_BYTES / DMEM_LINE_SIZE_BYTES,
                                    tx_tone_state[lane].fcw >> int_ratio_pow2[lane], tx_tone_state[lane].amplitude,
                                    &tx_tone_state[lane].phase);
            memset16(int_history, 0, sizeof(int_history)); // reset interpolator for new start
        }
        const uint16_t work_samples_count = DAC_XFER_SAMPLE_COUNT >> int_ratio_pow2[lane];
        interpol(dac_dest, ddr_src + tx->ready_buffer_offset, int_history, work_samples_count);
        tx->ready_buffer_offset += work_samples_count;

        // qec inplace
        if (txpipe[lane].generate_tone) {
            gen_nco_single_tone(dac_dest, DAC_XFER_SIZE_BYTES / DMEM_LINE_SIZE_BYTES,
                                tx_tone_state[lane].fcw >> int_ratio_pow2[lane], tx_tone_state[lane].amplitude,
                                &tx_tone_state[lane].phase);
        }
        tx_qec_correction(dac_dest, dac_dest, DAC_DMEM_LINES_COUNT);
        rhf_rhf_rsp_vMultiSclr_asm(dac_dest, dac_dest, &upscalefactor, DAC_XFER_SIZE_BYTES / DMEM_LINE_SIZE_BYTES);
        ddr_meta->phytime += DAC_XFER_SAMPLE_COUNT;
        dac_meta->flags &= ~IQSTREAM_TRX_END;
        TRACE_DURATION(T_TX_WORK, DEFAULT_THREAD_ID, t1);
        if (tx->ready_buffer_offset >= DDR_XFER_SAMPLE_COUNT) {
            ++tx->ddr.buffers_consumed;
            tx->consumed_slot = tx_ddr_next_slot(tx->consumed_slot);
            TRACE_COUNTER(CNT_TX_DDR_READY, ddr_tx_buffers_ready(&tx->ddr));
            tx->ready_buffer_offset = 0;
            dac_meta->flags |= ddr_meta->flags & IQSTREAM_TRX_END;
            tx_lane_try_ddr_enqueue(tx, true);
        }

    } else {
        if (txpipe[lane].generate_tone) {
            gen_nco_single_tone(ddr_src, DDR_XFER_SIZE_BYTES / DMEM_LINE_SIZE_BYTES,
                                tx_tone_state[lane].fcw >> int_ratio_pow2[lane], tx_tone_state[lane].amplitude,
                                &tx_tone_state[lane].phase);
        }
        tx_qec_correction(dac_dest, ddr_src, DAC_DMEM_LINES_COUNT);
        rhf_rhf_rsp_vMultiSclr_asm(dac_dest, dac_dest, &upscalefactor, DAC_XFER_SIZE_BYTES / DMEM_LINE_SIZE_BYTES);
        TRACE_DURATION(T_TX_WORK, DEFAULT_THREAD_ID, t1);
        ++tx->ddr.buffers_consumed;
        tx->consumed_slot = tx_ddr_next_slot(tx->consumed_slot);
        TRACE_COUNTER(CNT_TX_DDR_READY, ddr_tx_buffers_ready(&tx->ddr));
        tx_lane_try_ddr_enqueue(tx, true);
    }
    TRACE_COUNTER(CNT_DAC_PHYTIME_ADVANCE, dac_meta->phytime + stream_origin_phyticks - phytime_now());
    ++axiq->buffers_prepared;
    dac_enq_pending_buffer(dac);
}

static void dac_generate_tone_work(uint16_t lane, cfixed16_t *buf) {
    gen_nco_single_tone(buf, DAC_XFER_SIZE_BYTES / DMEM_LINE_SIZE_BYTES,
                        tx_tone_state[lane].fcw >> int_ratio_pow2[lane] // adjust fcw to host side expected sample rate
                        ,
                        tx_tone_state[lane].amplitude, &tx_tone_state[lane].phase);
    tx_qec_correction(buf, buf, DAC_DMEM_LINES_COUNT);
    dac_enque(&tx_dac[lane], buf, 0);
}

void dac_dma_complete(uint16_t lane) {
    TRACE_START_DURATION(t1);
    dac_pipeline_t *const dac = &tx_dac[lane];
    afe_pipeline_t *const afe = &dac->afe;
    ddr_pipeline_t *const ddr = &txddr[lane].ddr;
    TRACE_DMA_END(afe_dma_channel(afe), afe->count_dmac_complete & 1);

    const uint32_t dma_mask = 1 << afe_dma_channel(afe);
    dmac_clear_complete(dma_mask); // redundant, only using event bits
    dmac_clear_event(dma_mask);

    // TODO: use modulo buffer registers for circular addressing?
    cfixed16_t *const completed_buffer = afe_next_complete_buffer(afe);
    tx_meta_t *const dac_meta = dac_next_complete_meta(dac);
    ++afe->count_dmac_complete;
    ++tx_stats.afe_compl;

    if (txpipe[lane].generate_tone) {
        // generate samples and reenque immediately
        dac_generate_tone_work(0, completed_buffer);
        TRACE_DURATION(T_DAC_COMPLETE, DEFAULT_THREAD_ID, t1);
        return;
    }

    if ((dac_meta->flags & IQSTREAM_TRX_END) && dac->end_deferred) {
        dac->end_deferred = false;
        dac_schedule_end(dac, dac_meta);
    }

    // check if DDR has available data
    if (ddr_tx_buffers_ready(ddr) == 0) {
        // DDR depleted
        // `flags & IQSTREAM_TRX_END == 0` parses as `flags & (IQSTREAM_TRX_END == 0)`, which is always 0
        if ((dac_meta->flags & IQSTREAM_TRX_END) == 0) {
            ++tx_stats.dfe_udr;
        }

        TRACE_DURATION(T_DAC_COMPLETE, DEFAULT_THREAD_ID, t1);
        return;
    }

    // work will reenque next dac buffer
    ddr_to_dac_work(lane);

    TRACE_DURATION(T_DAC_COMPLETE, DEFAULT_THREAD_ID, t1);
}

// @brief Aborts and drops all current DDR buffers
static void tx_drop_ddr(uint16_t lane) {
    tx_ddr_pipeline_t *const tx = &txddr[lane];
    ddr_pipeline_t *const ddr = &tx->ddr;
    tx_stats.dfe_drop += ddr_tx_buffers_in_use(ddr);

    if (ddr->count_dmac_enque - ddr->count_dmac_complete > 0) {
        dmac_abort(1 << ddr->dma_channel);
        uint32_t in_flight_flags = 0;
        uint8_t slot = tx->cpl_slot;
        for (uint32_t i = ddr->count_dmac_complete; i != ddr->count_dmac_enque; ++i, slot = tx_ddr_next_slot(slot))
            in_flight_flags |= tx->meta[slot].flags;
        if (in_flight_flags & IQSTREAM_DMA_TCD_END) {
            ++ddr->dma.tcd_table.done;
            vspa_to_host_signal(ddr->dma.vth_tcd_done_flag_mask);
        }
    }

    // tx_stats.dfe_compl = 0;
    // tx_stats.dfe_enq = 0;
    ddr->count_dmac_enque = 0;
    ddr->count_dmac_complete = 0;
    ddr->buffers_consumed = 0;
    tx->enq_slot = tx->cpl_slot = tx->consumed_slot = 0;
    tx->ready_buffer_offset = 0;
    memset16(tx->meta, 0, sizeof(tx->meta));
}

void tx_ddr_complete(uint16_t lane) {
    TRACE_START_DURATION(t1);
    tx_ddr_pipeline_t *const tx = &txddr[lane];
    ddr_pipeline_t *const ddr = &tx->ddr;
    TRACE_DMA_END(ddr->dma_channel, ddr->count_dmac_complete & 1);
    dmac_clear_complete(1 << ddr->dma_channel);
    dmac_clear_event(1 << ddr->dma_channel);

    tx_meta_t *const ddr_meta = tx_ddr_next_completed_meta(tx);
    ++ddr->count_dmac_complete;
    tx->cpl_slot = tx_ddr_next_slot(tx->cpl_slot);
    ++tx_stats.dfe_compl;
    TRACE_COUNTER(CNT_TX_DDR_ENQ, ddr->count_dmac_enque - ddr->count_dmac_complete);
    TRACE_COUNTER(CNT_TX_DDR_READY, ddr_tx_buffers_ready(ddr));

    if (ddr_meta->flags & IQSTREAM_DMA_TCD_END) {
        ++ddr->dma.tcd_table.done;
        ++proxy.tx_dma_done;
        proxy_dirty = true;
        vspa_to_host_signal(ddr->dma.vth_tcd_done_flag_mask);
    }

    TRACE_COUNTER(CNT_DDR_CMPL_ADVANCE, ddr_meta->phytime + stream_origin_phyticks - phytime_now());
    afe_pipeline_t *const axiq = &tx_dac[lane].afe;
    if (afe_dac_buffers_in_use(axiq) < MAX_DMA_ENQ)
        ddr_to_dac_work(lane);
    // read ahead into the free ring slots while the DAC is still busy with earlier buffers
    tx_initial_ddr_enque(tx);

    TRACE_DURATION(T_DDR_RD_COMPLETE, DEFAULT_THREAD_ID, t1);
}

void transmitter_init(void) {
    tx_tdd.dac_allowed = 0; //-33; // over analog loopback there is 33 samples delay RxT0 = TxT33
    tx_tdd.pa_on_comparator_out = ePhyTimerComparatorOut1;
    tx_tdd.pa_off_comparator_out = ePhyTimerComparatorOut0;

    // don't change RF switch until configured which values should be used
    tx_tdd.rf_sw_on_comparator_out = ePhyTimerComparatorNoChange;
    tx_tdd.rf_sw_off_comparator_out = ePhyTimerComparatorNoChange;

    for (uint16_t lane = 0; lane < TX_MAX_LANE_COUNT; ++lane) {
        vspa_dma_hif_t *const dma_hif = &txddr[lane].ddr.dma;
        dma_hif->tcd_table.done = 0;
        dma_hif->htv_tcd_pending_flag_mask = (HTV_SIGNAL_TXLANE0_TCD_PENDING << lane);
        dma_hif->vth_tcd_done_flag_mask = (VTH_SIGNAL_TXLANE0_TCD_DONE << lane);
        tcd_fifo_reset(&dma_hif->tcd_table);
        clear_htv_signal(dma_hif->htv_tcd_pending_flag_mask);
        tx_set_sample_format(lane, IQ_SAMPLE_FORMAT_I16);
    }

    tx_lane_setup(0, 0);
}

// Takes effect when the lane streaming the channel is primed
int tx_set_oversampling(uint16_t channel, uint16_t oversample_pow2) {
    if (channel >= TX_CHANNEL_COUNT || oversample_pow2 > 2)
        return lime_Result_InvalidValue;

    tx_channel_cfg[channel].interpolate_pow2 = oversample_pow2;
    return lime_Result_Success;
}

int tx_set_sample_format(uint16_t lane, iq_sample_format_e format) {
    if (lane >= TX_MAX_LANE_COUNT)
        return lime_Result_InvalidValue;
    if (format == IQ_SAMPLE_FORMAT_I12)
        upscalefactor[lane] = 16.0f;
    else
        upscalefactor[lane] = 1.0f;
    return lime_Result_Success;
}

int tx_get_oversampling(uint16_t channel) {
    if (channel >= TX_CHANNEL_COUNT)
        return lime_Result_InvalidValue;
    return tx_channel_cfg[channel].interpolate_pow2;
}

static inline void tx_handle_dac_underrun(void) {
    TRACE_START_DURATION(t1);
    const uint16_t lane = 0;
    dac_pipeline_t *const dac = &tx_dac[lane];
    ddr_pipeline_t *const ddr = &txddr[lane].ddr;
    // Flush pending xfers and reset AXIQ fifo
    afe_pipeline_t *const afe = &dac->afe;

    tx_stats.afe_drop += afe_dac_buffers_in_use(afe);
    proxy.tx_drop += afe_dac_buffers_in_use(afe);
    afe_axiq_fifo_reset(afe);

    dac->in_progress = false;
    dac->end_scheduled = false;
    dac->end_deferred = false;
    dac->force_start = true;

    // discard following buffers that would be late
    uint32_t now = phytime_now();
    tx_ddr_pipeline_t *const tx = &txddr[lane];
    while (ddr_tx_buffers_ready(ddr) > 0) {
        tx_meta_t *const ddr_meta = tx_ddr_current_ready_meta(tx);
        const uint32_t tsAdvance = (ddr_meta->phytime + stream_origin_phyticks) - phytime_now();
        if (tsAdvance < reschedule_time || tsAdvance > 0x7FFFFFFFu) {
            // drop ddr buffer
            ++tx_stats.dfe_drop;
            ++proxy.tx_drop;
            ++tx->ddr.buffers_consumed;
            tx->consumed_slot = tx_ddr_next_slot(tx->consumed_slot);
            tx_lane_try_ddr_enqueue(tx, true);
            tx->ready_buffer_offset = 0;
            TRACE_COUNTER(CNT_TX_DDR_READY, ddr_tx_buffers_ready(ddr));
            continue;
        }

        ddr_to_dac_work(lane);
        if (int_ratio_pow2[lane] > 0) {
            // one ddr can fill multiple dac buffers
            ddr_to_dac_work(lane);
            TRACE_DURATION(T_DAC_AXIQ_RST, DEFAULT_THREAD_ID, t1);
            return;
        }
        break;
    }
    tx_initial_ddr_enque(lane);
    TRACE_DURATION(T_DAC_AXIQ_RST, DEFAULT_THREAD_ID, t1);
}

// Resets pipeline for streaming session
void tx_lane_prime(uint16_t lane) {
    tx_ddr_pipeline_t *const tx = &txddr[lane];
    dac_pipeline_t *const dac = &tx_dac[lane];
    memset16(&tx_stats, 0, sizeof(tx_stats));

    int_ratio_pow2[lane] = tx_channel_cfg[dac->channel].interpolate_pow2;
    tx_ddr_reset(tx, ddr_read_buffer[lane]);
    tx_host_dma_reset(tx);

    tx_dac_reset(dac, dac_buffer[lane]);

    TxAXIQ(true);
    afe_axiq_fifo_clear_err(dac->afe.config.axi_fifo_index);

    proxy.tx_udr = 0;
    proxy.tx_drop = 0;
    proxy_dirty = true;
}

void tx_lane_ddr_enable(uint16_t lane, bool enable) {
    if (enable)
        tx_lane_prime(lane);
}

// Aborts any pending transfers and resets the pipeline's AXIQ FIFO
void tx_lane_abort(uint16_t lane) {
    tx_drop_ddr(lane);
    afe_pipe_abort(&tx_dac[lane].afe);
}

int tx_tone_enable(uint16_t lane, bool enable) {
    if (txpipe[lane].generate_tone == enable)
        return 0;
    txpipe[lane].generate_tone = enable;

    if (enable) {
        dac_pipeline_t *const dac = &tx_dac[0];
        afe_pipeline_t *const afe = &tx_dac[0].afe;
        tx_dac_reset(dac, dac_buffer[lane]);
        int_ratio_pow2[lane] = tx_channel_cfg[dac->channel].interpolate_pow2;
        TxAXIQ(true);
        afe_axiq_fifo_clear_err(afe->config.axi_fifo_index);

        // initial work twice to produce two initial DAC buffers
        // no timestamps provided, will start immediately
        dac_generate_tone_work(lane, afe_next_enq_buffer(afe));
        dac_generate_tone_work(lane, afe_next_enq_buffer(afe));
    } else {
        tx_lane_abort(lane);
    }
    timer_trig_immediate(PHY_TIMER_COMP_CH5_TX_ALLOWED, enable ? ePhyTimerComparatorOut1 : ePhyTimerComparatorOut0);
    return 0;
}

void host_submitted_tx_tcd(uint16_t lane) {
    if (ddr_tx_buffers_in_use(&txddr[lane].ddr) >= TX_DDR_RING)
        return; // skip, all ddr buffers are in use

    // host only needs to signal the first TCD insertion to make VSPA go,
    // after that VSPA is self triggering with transfers completions.
    // Host can signal each insertion, but the extra GO events can upset DMA pacing.
    tx_initial_ddr_enque(&txddr[lane]);
}

static void tx_window_closed(dac_pipeline_t *dac);

// Close the window from the main loop (which runs on every Rx DMA completion) once the phytime passed its end, in case
// no GO_1 event came (END scheduled too late, GO_1 not re-armed). Without it the next START waits for ever.
static inline void tx_check_window_end(void) {
    dac_pipeline_t *const dac = &tx_dac[0];
    if (!(dac->in_progress && dac->end_scheduled))
        return;
    if ((int32_t)(phytime_now() - dac->end_go_phytime) < 0)
        return;
    ++tx_window_stats.go1_fallback;
    dac_stop_immediate(); // the END compare may have been missed too: Tx allowed must be low before the next START
    tx_window_closed(dac);
    timer_trig_immediate_async(PHY_TIMER_COMP_VSPA_GO_1, ePhyTimerComparatorOut0); // disarm a GO_1 that may still come
}

void tx_check_axiq_status(void) {
    tx_check_window_end();
    // TODO: interpolation path after underruns somehow end up with ddr buffers ready, but no events to trigger work
    if (ddr_tx_buffers_ready(&txddr[0].ddr) && !afe_dac_buffers_in_use(&tx_dac[0].afe))
        ddr_to_dac_work(0);

    // Check AXIQ tx fifo is not full or overrun
    uint32_t status = axiq_fifo_tx_sr(AXIQ_BANK_0, AXIQ_FIFO_TX0, AXIQ_SR_FIELD_ERROVER | AXIQ_SR_FIELD_ERRUNDER);
    if (status == 0)
        return;

    const uint8_t field_shift = axiq_sr_shift(AXIQ_FIFO_TX0);
    status >>= field_shift;
    if (status & AXIQ_SR_FIELD_ERROVER) {
        ++tx_stats.afe_ovr;
        TRACE_COUNTER(CNT_TX_AFE_OVR, tx_stats.afe_ovr);
    }
    if (status & AXIQ_SR_FIELD_ERRUNDER) {
        ++tx_stats.afe_udr;
        ++proxy.tx_udr;
        proxy_dirty = true;
        TRACE_COUNTER(CNT_TX_AFE_UDR, tx_stats.afe_udr);
        tx_handle_dac_underrun();
        return;
    }
    afe_axiq_fifo_clear_err(AXIQ_FIFO_TX0);
}

// The current transmission window has ended: allow the next START and enqueue the DAC buffers waiting for it.
static void tx_window_closed(dac_pipeline_t *dac) {
    dac->in_progress = false;
    dac->end_scheduled = false;
    dac->end_deferred = false;
    afe_pipeline_t *const axiq = &dac->afe;
    if (afe_dac_buffers_prepared(axiq) > 0) {
        dac_enq_pending_buffer(dac);
        if (afe_dac_buffers_prepared(axiq) > 0)
            dac_enq_pending_buffer(dac);
    }
}

void tx_external_trigger(void) {
    dac_pipeline_t *const dac = &tx_dac[0];
    // Only a GO_1 that belongs to a scheduled END closes the window; a late one after the phytime check already
    // closed it must not end the following transmission.
    if (dac->in_progress && dac->end_scheduled) {
        ++tx_window_stats.go1_ext;
        tx_window_closed(dac);
    }
    // need to clear trigger, go event is triggered only by rising edge
    timer_trig_immediate_async(PHY_TIMER_COMP_VSPA_GO_1, ePhyTimerComparatorOut0);
    iowr(EXT_GO_STAT, 0x2, 0x2); // clear external GO
}