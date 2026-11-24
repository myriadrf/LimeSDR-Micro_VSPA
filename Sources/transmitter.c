// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#include "transmitter.h"

#include "dmac.h"
#include "dfe.h"
#include "tone_generator.h"
#include "l1-trace.h"

#include "dma_common.h"
#include "iqstream_signals.h"
#include "interpolation/interpolation.h"

#include "opstatus.h"
#include "vspa_iqstream.h"
#include "phytimer.h"
#include "proxy_data.h"
#include "tdd.h"

#define TX_DAC_FIFO_BEAT_COUNT 16
#define dac_axi_fifo_addr (0x4400B000) // + 0x1000 - TX_DAC_FIFO_BEAT_COUNT * 16) // axi_DAC_FIFO_addr

#define DAC_XFER_SAMPLE_COUNT AXIQ_XFER_SAMPLES_COUNT
#define DAC_XFER_SIZE_BYTES (DAC_XFER_SAMPLE_COUNT * 4)
#define DAC_DMEM_LINES_COUNT (DAC_XFER_SIZE_BYTES / 128)

#define DDR_XFER_SAMPLE_COUNT AXIQ_XFER_SAMPLES_COUNT
#define DDR_XFER_SIZE_BYTES (DDR_XFER_SAMPLE_COUNT * 4)

extern uint32_t stream_origin_phyticks;
float upscalefactor[TX_PIPE_COUNT]; // when user sample format is only 12bit full scale, upscale to 16bits full scale

static const uint32_t reschedule_time = DAC_XFER_SAMPLE_COUNT / 2;
#define TX_LATE_TCD_DROP_MAX 64 // late TCDs dropped per call, bounds the time spent away from the main loop

// in VCPU DMEM: the IPPU DMEM holds the ADC buffers and the DDR read ring
// When the interpolator needs work memory (more than one kernel stage, x8 and up), the ring's last TX_DDR_RING_INT_SLOTS
// slots hold it instead: the VCPU DMEM has no room for it. At x8 one DDR buffer lasts 8 DAC buffers, so the shorter ring
// still holds far more DAC time than the full one does without interpolation.
#define TX_DDR_RING_INT_SLOTS ((INT_WORK_MEMORY_COUNT + DDR_XFER_SAMPLE_COUNT - 1) / DDR_XFER_SAMPLE_COUNT)
#if TX_DDR_RING_INT_SLOTS >= TX_DDR_RING - MAX_DMA_ENQ
#error "interpolator work memory leaves too few Tx DDR ring slots"
#endif
cfixed16_t dac_buffer[TX_PIPE_COUNT][MAX_DMA_ENQ * DAC_XFER_SAMPLE_COUNT] DMEM_LINES;
cfixed16_t ddr_read_buffer[TX_PIPE_COUNT][TX_DDR_RING * DDR_XFER_SAMPLE_COUNT] _VSPA_VECTOR_ALIGN
    __attribute__((section(".bss.ippu")));

tone_state_t tx_tone_state[TX_PIPE_COUNT] = { {
    .amplitude = 0.9,
    .phase = 0,
    .fcw = 0x7FFFFFFFu >> 2, // FS/8;
} };

#if TX_PIPE_COUNT > 1
#error "add tx_axiq_configs[] and tx_dac[] initializers for the extra pipes"
#endif
axiq_config_t tx_axiq_configs[TX_PIPE_COUNT] = {
    AXIQ_CONFIG_TX(TX0_AXI_FIFO_ADDR, TX_DAC_WR_DMA_CHANNEL, PHY_TIMER_COMP_CH5_TX_ALLOWED, AXIQ_FIFO_TX0),
};
dac_pipeline_t tx_dac[TX_PIPE_COUNT] = {
    { .afe.base_buffer = dac_buffer[0], .afe.config = &tx_axiq_configs[0] },
};
tx_ddr_pipeline_t tx_ddr[TX_PIPE_COUNT];
tx_pipeline_t tx_pipe[TX_PIPE_COUNT];
vspa_dma_hif_t tx_hdma[TX_PIPE_COUNT];

struct PipeStats tx_stats;
tx_tdd_config_t tx_tdd;

// Transmission window end diagnostics (host-readable in DMEM)
struct TxWindowStats {
    uint32_t go1_ext; // windows closed by the GO_1 phytimer event
    uint32_t go1_fallback; // windows closed by the phytime check because no GO_1 event came
    uint32_t end_late; // END was already in the past when it was scheduled (the compare can never match)
    uint32_t phytmr_busy; // a raw phytimer DMA found the channel still busy after the timeout
    uint32_t end_missed; // GO_1 came but Tx allowed was still high: the END compare did not fire, forced low
} tx_window_stats;

// Written by host through the features map (VSPA_CHANNEL_TX_CONFIG_ARRAY)
vspa_channel_config_t tx_channel_cfg[TX_CHANNEL_COUNT];

// Interpolation exponent of the channel the pipe streams
static inline uint16_t tx_pipe_exponent(uint16_t pipe) { return tx_channel_cfg[tx_dac[pipe].channel].oversample_exponent; }

// PHY timer ticks a DAC buffer of the channel lasts. With the DAC clock divider disabled, one tick is two DAC samples.
static inline uint32_t dac_buffer_phyticks(const dac_pipeline_t *dac) {
    return DAC_XFER_SAMPLE_COUNT >> tx_channel_cfg[dac->channel].clock_divider_disabled;
}

static inline tx_meta_t *tx_ddr_next_enq_meta(tx_ddr_pipeline_t *tx) { return &tx->meta[tx->enq_slot]; }
static inline tx_meta_t *tx_ddr_next_completed_meta(tx_ddr_pipeline_t *tx) { return &tx->meta[tx->cpl_slot]; }
static inline cfixed16_t *tx_ddr_current_ready_buffer(const tx_ddr_pipeline_t *tx) {
    return tx->ddr.base_buffer + tx->consumed_slot * DDR_XFER_SAMPLE_COUNT;
}

static inline tx_meta_t *tx_ddr_current_ready_meta(tx_ddr_pipeline_t *tx) { return &tx->meta[tx->consumed_slot]; }

static inline bool is_tx_dma_allowed(void) { return (gpird(1, 1 << 16)); }

// Converts user facing timestamp (samples count before interpolation) into DAC sample time (phytime ticks)
static inline uint32_t ts_to_phytime(uint32_t ts, const vspa_channel_config_t *cfg) {
    return (ts << cfg->oversample_exponent) >> cfg->clock_divider_disabled;
}

static inline cfixed16_t *tx_ddr_next_enq_buffer(const tx_ddr_pipeline_t *tx) {
    // TODO: use modulo buffer registers for circular addressing?
    return tx->ddr.base_buffer + tx->enq_slot * DDR_XFER_SAMPLE_COUNT;
}

static inline tx_meta_t *dac_next_enq_meta(dac_pipeline_t *pipe) { return &pipe->meta[pipe->afe.count_dmac_enque & 0x1]; }

static inline tx_meta_t *dac_next_free_meta(dac_pipeline_t *pipe) { return &pipe->meta[pipe->afe.buffers_prepared & 0x1]; }

static inline tx_meta_t *dac_next_complete_meta(dac_pipeline_t *pipe) { return &pipe->meta[pipe->afe.count_dmac_complete & 0x1]; }

static inline void dac_stop_immediate() { timer_trig_immediate_async(PHY_TIMER_COMP_CH5_TX_ALLOWED, ePhyTimerComparatorOut0); }

// @brief Attempt to enque all available DDR buffers (free ring slots, at most MAX_DMA_ENQ reads in flight)
static void tx_initial_ddr_enque(uint16_t pipe) {
    tx_ddr_pipeline_t *const tx = &tx_ddr[pipe];
    for (int i = 0; i < tx->ring_len; ++i) {
        const uint16_t in_use = ddr_tx_buffers_in_use(&tx->ddr);
        if (in_use >= tx->ring_len || ddr_xfer_in_flight(&tx->ddr) >= MAX_DMA_ENQ)
            break;
        tx_lane_try_ddr_enqueue(pipe, true);
        if (ddr_tx_buffers_in_use(&tx->ddr) == in_use)
            break; // nothing queued by the host
    }
}

static inline bool tx_axiq_is_enabled() {
    return gpord(7, (1 << 0)); // Tx AXIQ enable
}

static void tx_dac_reset(dac_pipeline_t *dac) {
    afe_pipe_reset(&dac->afe); // also clears the DMA channel's status bits

    dac->expected_end_phytime = 0;
    dac->in_progress = false;
    dac->end_scheduled = false;
    dac->end_deferred = false;
    dac->force_start = false;
    memset16(dac->meta, 0, sizeof(dac->meta));
}

static void tx_host_dma_reset(uint16_t pipe) {
    tx_hdma[pipe].tcd_table.done = 0;
    tcd_fifo_reset(&tx_hdma[pipe].tcd_table);
    proxy.tx_dma_done = 0;
    proxy_dirty = true;
}

// Empties the DDR ring. Its length depends on the channel's interpolation: from x8 up the interpolator takes the last
// ring slot(s) as its work memory, so the exponent must not cross that limit until the next reset.
static void tx_ddr_reset(uint16_t pipe) {
    tx_ddr_pipeline_t *const tx = &tx_ddr[pipe];
    timer_trig_immediate_async(PHY_TIMER_COMP_VSPA_GO_1, ePhyTimerComparatorOut0);
    ddr_pipe_reset(&tx->ddr, ddr_read_buffer[pipe]);
    tx->enq_slot = tx->cpl_slot = tx->consumed_slot = 0;
    tx->ready_buffer_offset = 0;
    tx->flush_remaining = 0;
    tx->ring_len = interpolation_needs_work_memory(tx_pipe_exponent(pipe)) ? TX_DDR_RING - TX_DDR_RING_INT_SLOTS : TX_DDR_RING;
    interpolation_clear(&int_pipe[pipe]);
    // memset16(tx->meta, 0, sizeof(tx->meta));

    const uint32_t dma_mask = (1 << tx->ddr.dma_channel);
    dmac_abort(dma_mask);
    WAIT_FOR(!dmac_is_running(dma_mask), VSPA_DEFAULT_TIMEOUT);

    dma_clear_all_bits(dma_mask);
}

int tx_select_channel(uint16_t pipe, e_tx_channel channel) {
    if (pipe >= TX_PIPE_COUNT || channel >= TX_CHANNEL_COUNT)
        return lime_Result_InvalidValue;
    memset16(&tx_stats, 0, sizeof(tx_stats));
    tx_dac[pipe].channel = channel;

    // VSPA_TX0 is the only Tx channel, its AXIQ config is fixed
    tx_dac_reset(&tx_dac[pipe]);
    // DAC DMA allowed comparator of the channel. The schedule table is shared, which is fine while there is one pipe.
    const uint16_t dac_allowed = tx_dac[pipe].afe.config->phytimer_id;
    tx_tdd_set_dac_allowed_comparator(dac_allowed);

    tx_ddr[pipe].ddr.dma_channel = DDR_RD_DMA_CHANNEL_1; // + pipe;
    tx_ddr_reset(pipe);
    tx_host_dma_reset(pipe);
    return lime_Result_Success;
}

void tx_lane_try_ddr_enqueue(uint16_t pipe, bool vcpu_trig) {
    TRACE_START_DURATION(t1);
    tx_ddr_pipeline_t *const tx = &tx_ddr[pipe];
    ddr_pipeline_t *const ddr = &tx->ddr;
    vspa_dma_hif_t *const hdma = &tx_hdma[pipe];

    if (tx_pipe[pipe].generate_tone)
        return;
    // ring full, or the DMA channel already has its maximum of reads in flight
    if (ddr_tx_buffers_in_use(ddr) >= tx->ring_len || ddr_xfer_in_flight(ddr) >= MAX_DMA_ENQ)
        return;

    const uint32_t tcd_count = tcd_fifo_size(&hdma->tcd_table);
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
    uint16_t late_tcds = 0;
    do {
        tcd = tcd_fifo_front(&hdma->tcd_table);
        // Drop TCDs that are already late as a whole, without reading them from the host. Otherwise every late buffer
        // is read, converted and only then dropped at the DAC stage, one per main loop pass: the pipeline works off
        // the backlog at real-time rate and a host delay of a few ms keeps Tx off the air for tens of ms.
        if ((tcd->flags & IQSTREAM_HAS_TIMESTAMP) && late_tcds < TX_LATE_TCD_DROP_MAX) {
            const uint32_t end_phytime =
                ts_to_phytime(tcd->timestamp_lsb + (tcd->size >> 2), &tx_channel_cfg[tx_dac[pipe].channel]) +
                stream_origin_phyticks;
            if ((int32_t)(end_phytime - phytime_now()) < (int32_t)reschedule_time) {
                tcd_fifo_pop(&hdma->tcd_table);
                ++hdma->tcd_table.done;
                ++proxy.tx_dma_done;
                ++proxy.tx_drop;
                proxy_dirty = true;
                ++tx_stats.dfe_drop;
                vspa_to_host_signal(hdma->vth_tcd_done_flag_mask);
                // the START of the burst may have been in a dropped TCD: start at the first buffer that is in time
                if (!tx_dac[pipe].in_progress)
                    tx_dac[pipe].force_start = true;
                ++late_tcds;
                if (tcd_fifo_size(&hdma->tcd_table) == 0)
                    return;
                continue;
            }
        }
        ddr_meta = tx_ddr_next_enq_meta(tx);

        // phytimer stream start offset will be applied right before scheduling.
        ddr_meta->phytime = ts_to_phytime(tcd->timestamp_lsb, &tx_channel_cfg[tx_dac[pipe].channel]);

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
    ddr_meta->samples = xfer_size >> 2;
    tcd->flags &= ~(IQSTREAM_TRX_START); // clear start tag once used

    if (tcd->size == 0) {
        ddr_meta->flags |= IQSTREAM_DMA_TCD_END;
        tcd_fifo_pop(&hdma->tcd_table);
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
    if (tx_pipe_exponent(pipe) == 0)
        dma_ctrl |= DMAC_MBRE; // if interpolation is not used, we're running at >40Msps, use burst mode.
#endif
    if (vcpu_trig)
        dma_ctrl |= DMAC_TRIG_VCPU;
    iowr(DMA_XFR_CTRL, dma_ctrl);
    TRACE_DMA_BEGIN(ddr->dma_channel, ddr->count_dmac_enque & 1);

    ++ddr->count_dmac_enque;
    tx->enq_slot = tx_ddr_next_slot(tx, tx->enq_slot);
    ++tx_stats.dfe_enq;
    TRACE_COUNTER(CNT_TX_DDR_ENQ, ddr->count_dmac_enque - ddr->count_dmac_complete);
    TRACE_DURATION(T_DDR_RD_ENQ, DEFAULT_THREAD_ID, t1);
}

inline static void dac_enque(dac_pipeline_t *dac, cfixed16_t *buffer, uint32_t extra_dmac_flags) {
    TRACE_START_DURATION(t1);
    afe_pipeline_t *const afe = &dac->afe;
    const axiq_config_t *axiq = afe->config;
    if (!dmac_is_available(1 << afe_dma_channel(afe))) {
        ++tx_stats.afe_err;
    }
    // when tx_dma_allowed is not yet triggered, DAC will immediately start to output
    // the first sample value of enqueued buffer.It will appear as DC until DMA is triggered.
    dmac_enable(axiq->dmac_ctrl_flags | DMAC_PEND_EXT | DMAC_TRIG_VCPU | extra_dmac_flags, DAC_XFER_SIZE_BYTES, axiq->axi_fifo_addr,
                VCPU_ADDR_FOR_DMA(buffer));
    TRACE_DMA_BEGIN(afe_dma_channel(afe), afe->count_dmac_enque & 1);

    ++afe->count_dmac_enque;
    ++tx_stats.afe_enq;
    TRACE_DURATION(T_DAC_ENQ, DEFAULT_THREAD_ID, t1);
}

// Schedule the end of the current transmission window at the end of this DAC buffer.
static inline void dac_schedule_end(dac_pipeline_t *pipe, const tx_meta_t *dac_meta) {
    uint32_t dac_off_phytime;
    if (dac_meta->flags & IQSTREAM_HAS_TIMESTAMP)
        dac_off_phytime = dac_meta->phytime + stream_origin_phyticks + dac_buffer_phyticks(pipe);
    else
        dac_off_phytime = pipe->expected_end_phytime + dac_buffer_phyticks(pipe);
    if ((int32_t)(dac_off_phytime - phytime_now()) <= 0) {
        // The END compare can never match a value that already passed: Tx allowed would stay high, no GO_1 would
        // come and the next START would have no rising edge. Stop now and let the window close.
        ++tx_window_stats.end_late;
        dac_stop_immediate();
    } else
        tx_tdd_schedule_end(&tx_tdd, dac_off_phytime, dac_off_phytime);
    pipe->end_scheduled = true;

    // current transmission window is active, and phytimers might be scheduled to trigger it's end.
    // Cannot schedule next window start, while the timers are still pending and
    // last DAC buffer DMA completion event triggers some time before the actual end of transmission,
    // so it cannot be used as phytimer rescheduling point. Need to schedule a dedicated GO event which
    // would trigger right after the actual transmission end and phytimers are done.
    pipe->end_go_phytime = dac_off_phytime;
    // GO_1 fires on a rising edge only: make sure the previous window's Out1 was cleared
    timer_trig_immediate_async(PHY_TIMER_COMP_VSPA_GO_1, ePhyTimerComparatorOut0);
    timer_trig_schedule_async(PHY_TIMER_COMP_VSPA_GO_1, ePhyTimerComparatorOut1, pipe->end_go_phytime);
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
                ++proxy.tx_drop; // visible to the host: without this its statistics show no loss for a dropped burst start
                proxy_dirty = true;
                --afe->buffers_prepared;
                // The dropped buffer carried the burst's START. Without it the following buffers of the burst would be
                // enqueued with no DAC start scheduled and wait for ever (Tx stall). Start the burst at its first buffer
                // that is still in time instead, as the underrun recovery does.
                pipe->force_start = true;
                return;
            }
        } else // transmit immediately
            tx_start_phytime = phytime_now() + reschedule_time;

        tx_tdd_schedule_start(&tx_tdd, tx_start_phytime);
        TRACE_EVENT(T_TX_BURST_START, 3, dac_meta->phytime);
        pipe->in_progress = true;
        pipe->start_phytime = tx_start_phytime;
        pipe->force_start = false;
        pipe->expected_end_phytime = tx_start_phytime + dac_buffer_phyticks(pipe);
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
        pipe->expected_end_phytime += dac_buffer_phyticks(pipe);
    const uint32_t dac_dma_flags = dac_meta->flags & IQSTREAM_TRX_END ? DMAC_FIFO_RESET : 0;
    dac_enque(pipe, afe_next_enq_buffer(afe), dac_dma_flags);
}

// DAC buffers of zero input needed after the burst's last DDR buffer, so that the interpolator outputs the whole response
// to the burst's samples. Without them the samples still in the filters' history are never output and the DAC holds its
// last value instead (e.g. at x2 the last 8 samples of a burst that ends within 8 samples of a DDR buffer's end).
static uint8_t tx_interpolation_flush_count(uint16_t exponent, uint16_t samples) {
    const int32_t outputs_past_buffer = ((int32_t)samples << exponent) + interpolation_tail_count(exponent) -
                                        ((int32_t)DDR_XFER_SAMPLE_COUNT << exponent);
    if (outputs_past_buffer <= 0)
        return 0;
    return (outputs_past_buffer + DAC_XFER_SAMPLE_COUNT - 1) / DAC_XFER_SAMPLE_COUNT;
}

static void ddr_to_dac_work(uint16_t pipe) {
    TRACE_START_DURATION(t1);
    tx_ddr_pipeline_t *const tx = &tx_ddr[pipe];
    dac_pipeline_t *const dac = &tx_dac[pipe];
    afe_pipeline_t *const afe = &tx_dac[pipe].afe;

    cfixed16_t *const ddr_src = tx_ddr_current_ready_buffer(tx);
    tx_meta_t *const ddr_meta = tx_ddr_current_ready_meta(tx);

    cfixed16_t *const dac_dest = afe_dac_next_free_buffer(afe);
    tx_meta_t *const dac_meta = dac_next_free_meta(dac);
    dac_meta->flags = ddr_meta->flags;
    dac_meta->phytime = ddr_meta->phytime;

    const uint16_t exponent = tx_pipe_exponent(pipe);
    if (exponent) {
        if (ddr_meta->flags & IQSTREAM_TRX_START) {
            ddr_meta->flags ^= IQSTREAM_TRX_START; // clear as ddr buffer is consumed over multiple interpolations
            interpolation_clear(&int_pipe[pipe]); // reset interpolator for new start
        }
        interpolate_work(&int_pipe[pipe], exponent, dac_dest, ddr_src + tx->ready_buffer_offset);
        tx->ready_buffer_offset += interpolation_input_count(exponent);

        // qec inplace
        tx_qec_correction(dac->channel, dac_dest, dac_dest, DAC_DMEM_LINES_COUNT);
        rhf_rhf_rsp_vMultiSclr_asm(dac_dest, dac_dest, &upscalefactor[pipe], DAC_XFER_SIZE_BYTES / DMEM_LINE_SIZE_BYTES);
        ddr_meta->phytime += dac_buffer_phyticks(dac);
        dac_meta->flags &= ~IQSTREAM_TRX_END;
        TRACE_DURATION(T_TX_WORK, DEFAULT_THREAD_ID, t1);
        bool consumed = tx->ready_buffer_offset >= DDR_XFER_SAMPLE_COUNT;
        if (tx->flush_remaining > 0) {
            // this output was interpolated from the zeroed buffer
            if (consumed)
                tx->ready_buffer_offset = 0; // still all zeros, keep reading it
            consumed = --tx->flush_remaining == 0;
        } else if (consumed && (ddr_meta->flags & IQSTREAM_TRX_END)) {
            // Burst's last DDR buffer: if its samples are still in the filters' history, keep the buffer as zero input
            // for the outputs that flush them, and END the burst with the last of those outputs instead of this one.
            tx->flush_remaining = tx_interpolation_flush_count(exponent, ddr_meta->samples);
            if (tx->flush_remaining > 0) {
                memset16(ddr_src, 0, DDR_XFER_SAMPLE_COUNT * sizeof(cfixed16_t));
                tx->ready_buffer_offset = 0;
                consumed = false;
            }
        }
        if (consumed) {
            ++tx->ddr.buffers_consumed;
            tx->consumed_slot = tx_ddr_next_slot(tx, tx->consumed_slot);
            TRACE_COUNTER(CNT_TX_DDR_READY, ddr_tx_buffers_ready(&tx->ddr));
            tx->ready_buffer_offset = 0;
            dac_meta->flags |= ddr_meta->flags & IQSTREAM_TRX_END;
            tx_lane_try_ddr_enqueue(pipe, true);
        }

    } else {
        tx_qec_correction(dac->channel, dac_dest, ddr_src, DAC_DMEM_LINES_COUNT);
        rhf_rhf_rsp_vMultiSclr_asm(dac_dest, dac_dest, &upscalefactor[pipe], DAC_XFER_SIZE_BYTES / DMEM_LINE_SIZE_BYTES);
        TRACE_DURATION(T_TX_WORK, DEFAULT_THREAD_ID, t1);
        ++tx->ddr.buffers_consumed;
        tx->consumed_slot = tx_ddr_next_slot(tx, tx->consumed_slot);
        TRACE_COUNTER(CNT_TX_DDR_READY, ddr_tx_buffers_ready(&tx->ddr));
        tx_lane_try_ddr_enqueue(pipe, true);
    }
    TRACE_COUNTER(CNT_DAC_PHYTIME_ADVANCE, dac_meta->phytime + stream_origin_phyticks - phytime_now());
    ++afe->buffers_prepared;
    dac_enq_pending_buffer(dac);
}

static void dac_generate_tone_work(uint16_t pipe, cfixed16_t *buf) {
    gen_nco_single_tone(buf, DAC_XFER_SIZE_BYTES / DMEM_LINE_SIZE_BYTES,
                        tx_tone_state[pipe].fcw >> tx_pipe_exponent(pipe) // adjust fcw to host side expected sample rate
                        ,
                        tx_tone_state[pipe].amplitude, &tx_tone_state[pipe].phase);
    tx_qec_correction(tx_dac[pipe].channel, buf, buf, DAC_DMEM_LINES_COUNT);
    dac_enque(&tx_dac[pipe], buf, 0);
}

void dac_dma_complete(uint16_t pipe) {
    TRACE_START_DURATION(t1);
    dac_pipeline_t *const dac = &tx_dac[pipe];
    afe_pipeline_t *const afe = &dac->afe;
    ddr_pipeline_t *const ddr = &tx_ddr[pipe].ddr;
    TRACE_DMA_END(afe_dma_channel(afe), afe->count_dmac_complete & 1);

    const uint32_t dma_mask = 1 << afe_dma_channel(afe);
    dmac_clear_complete(dma_mask); // redundant, only using event bits
    dmac_clear_event(dma_mask);

    // TODO: use modulo buffer registers for circular addressing?
    cfixed16_t *const completed_buffer = afe_next_complete_buffer(afe);
    tx_meta_t *const dac_meta = dac_next_complete_meta(dac);
    ++afe->count_dmac_complete;
    ++tx_stats.afe_compl;

    if (tx_pipe[pipe].generate_tone) {
        // generate samples and reenque immediately
        dac_generate_tone_work(pipe, completed_buffer);
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
        if ((dac_meta->flags & IQSTREAM_TRX_END) == 0) {
            ++tx_stats.dfe_udr;
        }

        TRACE_DURATION(T_DAC_COMPLETE, DEFAULT_THREAD_ID, t1);
        return;
    }

    // work will reenque next dac buffer
    ddr_to_dac_work(pipe);

    TRACE_DURATION(T_DAC_COMPLETE, DEFAULT_THREAD_ID, t1);
}

// @brief Aborts and drops all current DDR buffers
static void tx_drop_ddr(uint16_t pipe) {
    tx_ddr_pipeline_t *const tx = &tx_ddr[pipe];
    ddr_pipeline_t *const ddr = &tx->ddr;
    tx_stats.dfe_drop += ddr_tx_buffers_in_use(ddr);

    if (ddr->count_dmac_enque - ddr->count_dmac_complete > 0) {
        dmac_abort(1 << ddr->dma_channel);
        uint32_t in_flight_flags = 0;
        uint8_t slot = tx->cpl_slot;
        for (uint32_t i = ddr->count_dmac_complete; i != ddr->count_dmac_enque; ++i, slot = tx_ddr_next_slot(tx, slot))
            in_flight_flags |= tx->meta[slot].flags;
        if (in_flight_flags & IQSTREAM_DMA_TCD_END) {
            ++tx_hdma[pipe].tcd_table.done;
            vspa_to_host_signal(tx_hdma[pipe].vth_tcd_done_flag_mask);
        }
    }

    // tx_stats.dfe_compl = 0;
    // tx_stats.dfe_enq = 0;
    ddr->count_dmac_enque = 0;
    ddr->count_dmac_complete = 0;
    ddr->buffers_consumed = 0;
    tx->enq_slot = tx->cpl_slot = tx->consumed_slot = 0;
    tx->ready_buffer_offset = 0;
    tx->flush_remaining = 0;
    memset16(tx->meta, 0, sizeof(tx->meta));
    interpolation_clear(&int_pipe[pipe]);
}

void tx_ddr_complete(uint16_t pipe) {
    TRACE_START_DURATION(t1);
    tx_ddr_pipeline_t *const tx = &tx_ddr[pipe];
    ddr_pipeline_t *const ddr = &tx->ddr;
    TRACE_DMA_END(ddr->dma_channel, ddr->count_dmac_complete & 1);
    dmac_clear_complete(1 << ddr->dma_channel);
    dmac_clear_event(1 << ddr->dma_channel);

    tx_meta_t *const ddr_meta = tx_ddr_next_completed_meta(tx);
    ++ddr->count_dmac_complete;
    tx->cpl_slot = tx_ddr_next_slot(tx, tx->cpl_slot);
    ++tx_stats.dfe_compl;
    TRACE_COUNTER(CNT_TX_DDR_ENQ, ddr->count_dmac_enque - ddr->count_dmac_complete);
    TRACE_COUNTER(CNT_TX_DDR_READY, ddr_tx_buffers_ready(ddr));

    if (ddr_meta->flags & IQSTREAM_DMA_TCD_END) {
        ++tx_hdma[pipe].tcd_table.done;
        ++proxy.tx_dma_done;
        proxy_dirty = true;
        vspa_to_host_signal(tx_hdma[pipe].vth_tcd_done_flag_mask);
    }

    TRACE_COUNTER(CNT_DDR_CMPL_ADVANCE, ddr_meta->phytime + stream_origin_phyticks - phytime_now());
    afe_pipeline_t *const afe = &tx_dac[pipe].afe;
    if (afe_dac_buffers_in_use(afe) < MAX_DMA_ENQ)
        ddr_to_dac_work(pipe);
    // read ahead into the free ring slots while the DAC is still busy with earlier buffers
    tx_initial_ddr_enque(pipe);

    TRACE_DURATION(T_DDR_RD_COMPLETE, DEFAULT_THREAD_ID, t1);
}

void transmitter_init(void) {
    tx_tdd.dac_allowed = 0; //-33; // over analog loopback there is 33 samples delay RxT0 = TxT33
    tx_tdd.pa_on_comparator_out = ePhyTimerComparatorOut1;
    tx_tdd.pa_off_comparator_out = ePhyTimerComparatorOut0;

    // don't change RF switch until configured which values should be used
    tx_tdd.rf_sw_on_comparator_out = ePhyTimerComparatorNoChange;
    tx_tdd.rf_sw_off_comparator_out = ePhyTimerComparatorNoChange;

    for (uint16_t pipe = 0; pipe < TX_PIPE_COUNT; ++pipe) {
        vspa_dma_hif_t *const dma_hif = &tx_hdma[pipe];
        dma_hif->tcd_table.done = 0;
        dma_hif->htv_tcd_pending_flag_mask = (HTV_SIGNAL_TXLANE0_TCD_PENDING << pipe);
        dma_hif->vth_tcd_done_flag_mask = (VTH_SIGNAL_TXLANE0_TCD_DONE << pipe);
        tcd_fifo_reset(&dma_hif->tcd_table);
        clear_htv_signal(dma_hif->htv_tcd_pending_flag_mask);
        tx_set_sample_format(pipe, IQ_SAMPLE_FORMAT_I16);
    }

    for (uint16_t pipe = 0; pipe < TX_PIPE_COUNT; ++pipe) {
        // interpolator work memory: the last ring slots, used only when interpolation_needs_work_memory()
        interpolation_setup(&int_pipe[pipe], ddr_read_buffer[pipe] + (TX_DDR_RING - TX_DDR_RING_INT_SLOTS) * DDR_XFER_SAMPLE_COUNT);
    }
    tx_select_channel(0, VSPA_TX0);
}

int tx_set_sample_format(uint16_t pipe, iq_sample_format_e format) {
    if (pipe >= TX_PIPE_COUNT)
        return lime_Result_InvalidValue;
    if (format == IQ_SAMPLE_FORMAT_I12)
        upscalefactor[pipe] = 16.0f;
    else
        upscalefactor[pipe] = 1.0f;
    return lime_Result_Success;
}

static inline void tx_handle_dac_underrun(void) {
    TRACE_START_DURATION(t1);
    const uint16_t pipe = 0;
    dac_pipeline_t *const dac = &tx_dac[pipe];
    ddr_pipeline_t *const ddr = &tx_ddr[pipe].ddr;
    // Flush pending xfers and reset AXIQ fifo
    afe_pipeline_t *const afe = &dac->afe;

    tx_stats.afe_drop += afe_dac_buffers_in_use(afe);
    proxy.tx_drop += afe_dac_buffers_in_use(afe);
    axiq_pipe_reset(afe);
    // The aborted DAC transfers never complete: without clearing the counters the pipeline stays "full" and no further
    // DDR buffer is moved to the DAC.
    afe->count_dmac_enque = 0;
    afe->count_dmac_complete = 0;
    afe->buffers_prepared = 0;
    // axiq_pipe_reset() leaves the Tx AXIQ FIFO disabled (falling edge = flush mode): enable it again, as
    // tx_lane_prime() does, otherwise the restarted burst is never clocked out.
    axiq_pipe_enable(dac->afe.config);

    dac->in_progress = false;
    dac->end_scheduled = false;
    dac->end_deferred = false;
    dac->force_start = true;

    // discard following buffers that would be late
    uint32_t now = phytime_now();
    tx_ddr_pipeline_t *const tx = &tx_ddr[pipe];
    while (ddr_tx_buffers_ready(ddr) > 0) {
        tx_meta_t *const ddr_meta = tx_ddr_current_ready_meta(tx);
        const uint32_t tsAdvance = (ddr_meta->phytime + stream_origin_phyticks) - phytime_now();
        if (tsAdvance < reschedule_time || tsAdvance > 0x7FFFFFFFu) {
            // drop ddr buffer
            ++tx_stats.dfe_drop;
            ++proxy.tx_drop;
            ++tx->ddr.buffers_consumed;
            tx->consumed_slot = tx_ddr_next_slot(tx, tx->consumed_slot);
            tx_lane_try_ddr_enqueue(pipe, true);
            tx->ready_buffer_offset = 0;
            tx->flush_remaining = 0; // a dropped burst end needs no flush
            interpolation_clear(&int_pipe[pipe]); // samples buffered from the dropped buffer must not be output
            TRACE_COUNTER(CNT_TX_DDR_READY, ddr_tx_buffers_ready(ddr));
            continue;
        }

        ddr_to_dac_work(pipe);
        if (tx_pipe_exponent(pipe) > 0) {
            // one ddr can fill multiple dac buffers
            ddr_to_dac_work(pipe);
            TRACE_DURATION(T_DAC_AXIQ_RST, DEFAULT_THREAD_ID, t1);
            return;
        }
        break;
    }
    tx_initial_ddr_enque(pipe);
    TRACE_DURATION(T_DAC_AXIQ_RST, DEFAULT_THREAD_ID, t1);
}

// Resets pipeline for streaming session
void tx_lane_prime(uint16_t pipe) {
    tx_ddr_pipeline_t *const tx = &tx_ddr[pipe];
    dac_pipeline_t *const dac = &tx_dac[pipe];
    memset16(&tx_stats, 0, sizeof(tx_stats));

    tx_ddr_reset(pipe);
    tx_host_dma_reset(pipe);

    tx_dac_reset(dac);

    axiq_pipe_enable(dac->afe.config);

    proxy.tx_udr = 0;
    proxy.tx_drop = 0;
    proxy_dirty = true;
}

void tx_lane_ddr_enable(uint16_t pipe, bool enable) {
    if (enable)
        tx_lane_prime(pipe);
}

// Aborts any pending transfers and resets the pipeline's AXIQ FIFO
void tx_lane_abort(uint16_t pipe) {
    tx_drop_ddr(pipe);
    afe_pipe_abort(&tx_dac[pipe].afe);
}

int tx_tone_enable(uint16_t pipe, bool enable) {
    if (pipe >= TX_PIPE_COUNT)
        return lime_Result_InvalidValue;
    if (tx_pipe[pipe].generate_tone == enable)
        return 0;
    tx_pipe[pipe].generate_tone = enable;

    if (enable) {
        dac_pipeline_t *const dac = &tx_dac[pipe];
        afe_pipeline_t *const afe = &dac->afe;
        const axiq_config_t *const axiq = afe->config;
        tx_dac_reset(dac);
        axiq_pipe_enable(dac->afe.config);

        // initial work twice to produce two initial DAC buffers
        // no timestamps provided, will start immediately
        dac_generate_tone_work(pipe, afe_next_enq_buffer(afe));
        dac_generate_tone_work(pipe, afe_next_enq_buffer(afe));
    } else {
        tx_lane_abort(pipe);
    }
    timer_trig_immediate_async(tx_dac[pipe].afe.config->phytimer_id, enable ? ePhyTimerComparatorOut1 : ePhyTimerComparatorOut0);
    return 0;
}

extern void ProcessTxDMA(void);

// Tone buffers get QEC applied when generated, and up to MAX_DMA_ENQ of them are queued ahead of the DAC.
// Wait until the buffers queued before this call (+1 for the AXIQ FIFO contents) have been sent, so that the
// DAC output reflects the current tone and Tx QEC settings.
void tx_tone_flush(uint16_t pipe) {
    if (!tx_pipe[pipe].generate_tone)
        return;
    const afe_pipeline_t *const afe = &tx_dac[pipe].afe;
    const uint32_t target = afe->count_dmac_complete + MAX_DMA_ENQ + 1;
    volatile int32_t timeout = 2000 * (MAX_DMA_ENQ + 1);
    while ((int32_t)(target - afe->count_dmac_complete) > 0 && --timeout)
        ProcessTxDMA();
}

void host_submitted_tx_tcd(uint16_t pipe) {
    if (ddr_tx_buffers_in_use(&tx_ddr[pipe].ddr) >= tx_ddr[pipe].ring_len)
        return; // skip, all ddr buffers are in use

    // host only needs to signal the first TCD insertion to make VSPA go,
    // after that VSPA is self triggering with transfers completions.
    // Host can signal each insertion, but the extra GO events can upset DMA pacing.
    tx_initial_ddr_enque(pipe);
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
    if (ddr_tx_buffers_ready(&tx_ddr[0].ddr) && !afe_dac_buffers_in_use(&tx_dac[0].afe))
        ddr_to_dac_work(0);

    afe_pipeline_t *const axiq = &tx_dac[0].afe;
    if (axiq_fifo_errors(axiq)) {
        ++tx_stats.afe_udr;
        ++proxy.tx_udr;
        proxy_dirty = true;
        TRACE_COUNTER(CNT_TX_AFE_UDR, tx_stats.afe_udr);
        tx_handle_dac_underrun();
        return;
    }
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
        // GO_1 is scheduled after the END compare. If Tx allowed is still high here, the END compare did not fire
        // (the comparators match on equality only, and the END is scheduled just one or two DAC buffers ahead, so
        // its value can pass before the write completes). Without a falling edge the next START has no rising edge:
        // its DAC transfers would wait for their trigger for ever (Tx stall with no underrun and nothing dropped).
        if (is_tx_dma_allowed()) {
            ++tx_window_stats.end_missed;
            dac_stop_immediate();
        }
        tx_window_closed(dac);
    }
    // need to clear trigger, go event is triggered only by rising edge
    timer_trig_immediate_async(PHY_TIMER_COMP_VSPA_GO_1, ePhyTimerComparatorOut0);
    iowr(EXT_GO_STAT, 0x2, 0x2); // clear external GO
}