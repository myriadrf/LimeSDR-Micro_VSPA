// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#include "receiver.h"
#include "transmitter.h"

#include "dmac.h"
#include "dfe.h"
#include "tone_generator.h"
#include "l1-trace.h"

#include "dma_common.h"
#include "iqstream_signals.h"
#include "vspa_iqstream.h"
#include "phytimer.h"
#include "proxy_data.h"
#include "nco_correlator.h"
#include "decimation/decimation.h"

#include "ditfft.h"

#include "opstatus.h"

#define ADC_XFER_SAMPLE_COUNT AXIQ_XFER_SAMPLES_COUNT
#if ADC_XFER_SAMPLE_COUNT != DEC_INPUT_COUNT
#error "decimation cascades are sized for a different ADC transfer size"
#endif
#define ADC_XFER_SIZE_BYTES (ADC_XFER_SAMPLE_COUNT * 4)
#define ADC_DMEM_LINES_COUNT (ADC_XFER_SIZE_BYTES / 128)

#define DDR_XFER_SAMPLE_COUNT AXIQ_XFER_SAMPLES_COUNT
#define DDR_XFER_SIZE_BYTES (DDR_XFER_SAMPLE_COUNT * 4)

// How far in the future sampling restart is scheduled, doubled on each failed attempt
#define RX_RESYNC_LEAD_TICKS 1024
#define RX_RESYNC_MAX_ATTEMPTS 6

extern uint32_t stream_origin_phyticks;
static float
    downscalefactor[RX_PIPE_COUNT]; // when user sample format is 12bit fullscale, downscale from ADC 16bit fullscale to 12bit

cfixed16_t adc_buffer[RX_PIPE_COUNT][MAX_DMA_ENQ * ADC_XFER_SAMPLE_COUNT] __attribute__((section(".bss.ippu"))) _VSPA_VECTOR_ALIGN;
cfixed16_t ddr_write_buffer[RX_PIPE_COUNT][MAX_DMA_ENQ * DDR_XFER_SAMPLE_COUNT]
    __attribute__((section(".bss.vcpu"))) _VSPA_VECTOR_ALIGN;

// Decimation filter state

tone_state_t rx_generator[RX_PIPE_COUNT];

struct PipeStats rx_stats[RX_PIPE_COUNT];

dma_tcd_fifo_t rx_output_blocks[RX_PIPE_COUNT];
vspa_dma_hif_t rx_hdma[RX_PIPE_COUNT];
dma_tcd_t rx_wip_tcd[RX_PIPE_COUNT]; // working copy of input tcd, so the original could be recirculated.

ddr_pipeline_t rx_ddr[RX_PIPE_COUNT];
rx_pipeline_t rx_pipe[RX_PIPE_COUNT];
#if RX_PIPE_COUNT > 2
#error "add rx_axiq_configs[] and rx_adc[] initializers for the extra pipes"
#endif
// Set on channel select. Defaults match receiver_init(): pipe N streams VSPA_RX0 + N
axiq_config_t rx_axiq_configs[RX_PIPE_COUNT] = {
    AXIQ_CONFIG_RX(RX0_AXI_FIFO_ADDR, RX0_ADC_RD_DMA_CHANNEL, PHY_TIMER_COMP_CH3_RX_ALLOWED, AXIQ_FIFO_RX2),
#if RX_PIPE_COUNT > 1
    AXIQ_CONFIG_RX(RX1_AXI_FIFO_ADDR, RX1_ADC_RD_DMA_CHANNEL, PHY_TIMER_COMP_CH4_RX_ALLOWED, AXIQ_FIFO_RX3),
#endif
};
adc_pipeline_t rx_adc[RX_PIPE_COUNT] = {
    { .afe.base_buffer = adc_buffer[0], .afe.config = &rx_axiq_configs[0] },
#if RX_PIPE_COUNT > 1
    { .afe.base_buffer = adc_buffer[1], .afe.config = &rx_axiq_configs[1] },
#endif
};

// Written by host through the features map (VSPA_CHANNEL_RX_CONFIG_ARRAY)
vspa_channel_config_t rx_channel_cfg[RX_CHANNEL_COUNT];

static inline const vspa_channel_config_t *rx_pipe_cfg(uint16_t pipe) { return &rx_channel_cfg[rx_adc[pipe].channel]; }
static inline uint16_t rx_pipe_exponent(uint16_t pipe) { return rx_pipe_cfg(pipe)->oversample_exponent; }

// Converts ADC sample time (phytime ticks since stream origin) into user facing timestamp (samples count after decimation).
// With the ADC clock divider disabled, one PHY timer tick is two ADC samples.
static inline uint64_t phyticks_to_ts(uint64_t phyticks, uint16_t oversample_exponent, uint16_t clock_divider_disabled) {
    return (phyticks << clock_divider_disabled) >> oversample_exponent;
}

static inline void rx_wip_tcd_load(uint16_t pipe) {
    rx_pipeline_t *const rx = &rx_pipe[pipe];
    rx_wip_tcd[pipe] = *tcd_fifo_front(&rx_hdma[pipe].tcd_table);
    rx->wip_tcd_output = tcd_fifo_back(&rx_output_blocks[pipe]);
    dma_tcd_reset(rx->wip_tcd_output);
}

// Completes TCD that is currently being filled. Status is published to host once the last
// data transfer into the TCD buffer completes.
static void rx_wip_tcd_finish(uint16_t pipe, uint32_t extra_flags) {
    // timestamp and size are filled in while the data transfers are enqueued
    rx_pipe[pipe].wip_tcd_output->flags |= extra_flags;
    tcd_fifo_push(&rx_output_blocks[pipe]); // mark output as ready

    // recirculate TCD to the back of the queue, host buffers are reused continuously
    *(tcd_fifo_back(&rx_hdma[pipe].tcd_table)) = *tcd_fifo_front(&rx_hdma[pipe].tcd_table);
    tcd_fifo_pop(&rx_hdma[pipe].tcd_table);
    tcd_fifo_push(&rx_hdma[pipe].tcd_table);
    rx_wip_tcd_load(pipe);

    // Mark input TCD as completed
    ++rx_hdma[pipe].tcd_table.done;
    ++proxy.rx_dma_done;
    proxy_dirty = true;
    vspa_to_host_signal(rx_hdma[pipe].vth_tcd_done_flag_mask);
}

// Samples following the ones already written are lost. TCD buffer must contain only contiguous samples,
// so partially filled TCD is completed early.
static void rx_wip_tcd_discontinuity(uint16_t pipe) {
    if (tcd_is_valid(&rx_wip_tcd[pipe]))
        rx_wip_tcd_finish(pipe, IQSTREAM_TRUNCATED);
}

static inline void rx_adc_pipe_reset(adc_pipeline_t *adc) {
    afe_pipe_reset(&adc->afe);
    adc->phyticks = 0;
    adc->sampling = false;
}

static inline void rx_ddr_pipe_reset(uint16_t pipe, cfixed16_t *buffer) {
    ddr_pipeline_t *const ddr = &rx_ddr[pipe];
    ddr_pipe_reset(ddr, buffer);
    rx_pipe[pipe].buf_timestamp = 0;

    rx_wip_tcd[pipe].size = 0;
    rx_wip_tcd[pipe].flags = 0;

    tcd_fifo_reset(&rx_hdma[pipe].tcd_table);
    tcd_fifo_reset(&rx_output_blocks[pipe]);
    rx_pipe[pipe].wip_tcd_output = tcd_fifo_back(&rx_output_blocks[pipe]);
    dma_tcd_reset(rx_pipe[pipe].wip_tcd_output);
    // memset16(ddr->meta, 0, sizeof(ddr->meta));

    const uint32_t dma_mask = (1 << ddr->dma_channel);
    dmac_abort(dma_mask);
    WAIT_FOR(!dmac_is_running(dma_mask), VSPA_DEFAULT_TIMEOUT);
    dma_clear_all_bits(dma_mask);
}

int rx_select_channel(uint16_t pipe, e_rx_channel channel) {
    if (pipe >= RX_PIPE_COUNT || channel > VSPA_RX1)
        return lime_Result_InvalidValue;
    adc_pipeline_t *const adc = &rx_adc[pipe];
    adc->channel = channel;

    rx_axiq_fill_config(&rx_axiq_configs[pipe], channel);
    rx_adc_pipe_reset(&rx_adc[pipe]);

    ddr_pipeline_t *const ddr = &rx_ddr[pipe];
    ddr->dma_channel = DDR_WR_DMA_CHANNEL_1 + pipe;
    rx_ddr_pipe_reset(pipe, ddr_write_buffer[pipe]);

    vspa_dma_hif_t *const hdma = &rx_hdma[pipe];

    hdma->htv_tcd_pending_flag_mask = (HTV_SIGNAL_RXLANE0_TCD_PENDING << pipe);
    hdma->vth_tcd_done_flag_mask = (VTH_SIGNAL_RXLANE0_TCD_DONE << pipe);

    clear_htv_signal(hdma->htv_tcd_pending_flag_mask);
    return lime_Result_Success;
}

int rx_set_sample_format(uint16_t pipe, iq_sample_format_e format) {
    if (pipe >= RX_PIPE_COUNT)
        return lime_Result_InvalidValue;
    if (format == IQ_SAMPLE_FORMAT_I12)
        downscalefactor[pipe] = 0.0625f;
    else
        downscalefactor[pipe] = 1.0f;
    return lime_Result_Success;
}

// Prime ADC AXIQ and DMA engine, the actual start is triggered by phytimer
static inline void initial_adc_enq(afe_pipeline_t *afe) {
    const axiq_config_t *const axiq = afe->config;

    axiq_pipe_enable(axiq);
    // Enabling Rx AXIQ instantly generates underrun error, clear it.
    axiq_pipe_clear_err(axiq);

    // enque two reads
    const uint32_t dma_ctrl = axiq->dmac_ctrl_flags | DMAC_TRIG_VCPU | DMAC_PEND_EXT;
    dmac_prep_a_s(ADC_XFER_SIZE_BYTES, axiq->axi_fifo_addr);
    dmac_enable_v_c(dma_ctrl, VCPU_ADDR_FOR_DMA(afe->base_buffer));
    TRACE_DMA_BEGIN(afe_dma_channel(afe), afe->count_dmac_enque & 1);
    // ++rx_stats[pipe].afe_enq;
    ++afe->count_dmac_enque;
    dmac_enable_v_c(dma_ctrl, VCPU_ADDR_FOR_DMA(&afe->base_buffer[AXIQ_XFER_SAMPLES_COUNT]));
    // TRACE_DMA_BEGIN(axiq->dma_channel, rx_stats[pipe].afe_enq & 1);
    // ++rx_stats[pipe].afe_enq;
    ++afe->count_dmac_enque;
}

// @param timestamp of the first sample in the buffer
static void rx_lane_try_ddr_enqueue(uint16_t pipe, const cfixed16_t *buffer, uint64_t timestamp) {
    TRACE_START_DURATION(t1);
    rx_pipeline_t *const rx = &rx_pipe[pipe];
    ddr_pipeline_t *const ddr = &rx_ddr[pipe];
    if (!dmac_is_available(1 << ddr->dma_channel)) {
        // should not happen. DDR transfers should complete faster than ADC acquisition.
        // IF ddr is not available, then realtime work is not possible.
        ++rx_stats[pipe].dfe_ovr;
        ++proxy.rx_ovr;
        proxy_dirty = true;
        rx_wip_tcd_discontinuity(pipe);
        return;
    }

    if (!tcd_is_valid(&rx_wip_tcd[pipe])) {
        if (tcd_fifo_isempty(&rx_hdma[pipe].tcd_table)) {
            ++rx_stats[pipe].dfe_udr;
            rx_wip_tcd_discontinuity(pipe);
            return;
        }
        rx_wip_tcd_load(pipe);
    }

    volatile dma_tcd_t *const tcd = &rx_wip_tcd[pipe];
    const uint32_t xfer_size = tcd->size > DDR_XFER_SIZE_BYTES ? DDR_XFER_SIZE_BYTES : tcd->size;

    iowr(DMA_DMEM_PRAM_ADDR, VCPU_ADDR_FOR_DMA(buffer));
    iowr(DMA_AXI_ADDRESS, tcd->la9310_mem_address);
    iowr(DMA_AXI_BYTE_CNT, xfer_size);

    if (rx->wip_tcd_output->size == 0) {
        // first samples going into the TCD buffer
        rx->wip_tcd_output->la9310_mem_address = tcd->la9310_mem_address;
        rx->wip_tcd_output->timestamp_lsb = (uint32_t)timestamp;
        rx->wip_tcd_output->timestamp_msb = (uint32_t)(timestamp >> 32);
    }

    tcd->la9310_mem_address += xfer_size;
    tcd->size -= xfer_size;
    rx->wip_tcd_output->size += xfer_size;

    meta_t *const ddr_meta = ddr_next_enq_meta(ddr);
    ddr_meta->flags = tcd->flags;

    // DDR writes complete faster than ADC reads, so no need for DMAC_TRIG_VCPU
    // VCPU will be triggered only by ADC transfers to maintain consistent pacing
    const uint32_t dma_ctrl = DMAC_WRC | ddr->dma_channel;
    iowr(DMA_XFR_CTRL, dma_ctrl);
    TRACE_DMA_BEGIN(ddr->dma_channel, ddr->count_dmac_enque & 1);
    ++ddr->count_dmac_enque;
    rx_stats[pipe].dfe_enq = ddr->count_dmac_enque;

    if (tcd->size == 0) {
        ddr_meta->flags |= IQSTREAM_DMA_TCD_END;
    }

    TRACE_DURATION(T_DDR_WR_ENQ, DEFAULT_THREAD_ID, t1);
}

// Restarts sampling at a known phytime to reestablish samples timing. Samples that are still in the
// ADC path are dropped, already sent to DDR remain valid, as they were all received before the overrun.
static void rx_resync(uint16_t pipe) {
    TRACE_START_DURATION(t1);
    adc_pipeline_t *const adc = &rx_adc[pipe];
    afe_pipeline_t *const afe = &rx_adc[pipe].afe;
    const axiq_config_t *const axiq = afe->config;
    decimation_pipeline_t *const dec = &dec_pipe[pipe];

    // partially accumulated DDR block is dropped, filter history is not contiguous with restarted samples
    decimation_clear(&dec_pipe[pipe]);
    rx_wip_tcd_discontinuity(pipe);

    // keep restarted samples on the decimated samples grid
    const uint32_t align_mask = (1 << rx_pipe_exponent(pipe)) - 1;
    // ADC sample time of the restart, if no samples would have been lost
    const uint32_t expected_phytime = stream_origin_phyticks + (uint32_t)adc->phyticks;

    uint32_t start_phytime;
    uint32_t lead = RX_RESYNC_LEAD_TICKS;
    for (uint16_t attempt = 0;; ++attempt, lead <<= 1) {
        axiq_pipe_reset(afe);
        rx_adc_pipe_reset(adc);
        initial_adc_enq(afe);

        if (attempt == RX_RESYNC_MAX_ATTEMPTS) {
            // Could not schedule in time, start immediately and estimate the time
            const uint32_t before_phytime = phytime_now();
            timer_trig_immediate_async(axiq->phytimer_id, ePhyTimerComparatorOut1);
            start_phytime = before_phytime + ((phytime_now() - before_phytime) >> 1);
            break;
        }

        const uint32_t start_offset = ((phytime_now() + lead - stream_origin_phyticks) + align_mask) & ~align_mask;
        start_phytime = stream_origin_phyticks + start_offset;
        timer_trig_schedule_async(axiq->phytimer_id, ePhyTimerComparatorOut1, start_phytime);
        timer_wait_done();
        // Comparator triggers only on equality, if start time has already passed it might have been missed.
        if ((int32_t)(phytime_now() - start_phytime) < 0)
            break;
    }

    adc->phyticks += (int32_t)(start_phytime - expected_phytime);
    adc->sampling = true;
    ++rx_stats[pipe].afe_drop;
    TRACE_DURATION(T_ADC_AXIQ_RST, DEFAULT_THREAD_ID, t1);
}

void adc_dma_complete(uint16_t pipe) {
    adc_pipeline_t *const adc = &rx_adc[pipe];
    afe_pipeline_t *const afe = &rx_adc[pipe].afe;

    TRACE_START_DURATION(t1);
    TRACE_DMA_END(afe_dma_channel(afe), afe->count_dmac_complete & 1);

    const uint16_t dma_mask = 1 << afe_dma_channel(afe);
    dmac_clear_complete(dma_mask); // don't care about complete, checking only event
    dmac_clear_event(dma_mask);

    if (axiq_fifo_errors(afe)) {
        // Unknown amount of samples were lost, completed buffer might already contain the gap.
        rx_resync(pipe);
        TRACE_DURATION(T_ADC_COMPLETE, DEFAULT_THREAD_ID, t1);
        return;
    }

    cfixed16_t *const completed_buffer = afe_next_complete_buffer(afe);
    ++afe->count_dmac_complete;
    ++rx_stats[pipe].afe_compl;

    // read once, the host may write the config at any time
    const vspa_channel_config_t *const cfg = rx_pipe_cfg(pipe);
    const uint16_t exponent = cfg->oversample_exponent;
    const uint16_t clock_divider_disabled = cfg->clock_divider_disabled;

    const uint64_t buffer_time = adc->phyticks;
    adc->phyticks += (AXIQ_XFER_SAMPLES_COUNT >> clock_divider_disabled);

    // work
    rx_pipeline_t *const rx = &rx_pipe[pipe];
    ddr_pipeline_t *const ddr = &rx_ddr[pipe];
    decimation_pipeline_t *const dec = &dec_pipe[pipe];
    cfixed16_t *const dest = ddr_next_enq_buffer(ddr);
    TRACE_START_DURATION(t2);
    if (dec->samples_produced == 0) {
        rx->buf_timestamp = phyticks_to_ts(buffer_time, exponent, clock_divider_disabled);
    }
    rx_qec_correction(rx_adc[pipe].channel, completed_buffer, completed_buffer, ADC_DMEM_LINES_COUNT);

    cfixed16_t *buffer_to_scale;
    if (exponent) {
        // in place processing
        decimate_work(dec, exponent, dest + dec->samples_produced, completed_buffer, ADC_XFER_SAMPLE_COUNT);
        buffer_to_scale = dest;
    } else {
        dec->samples_produced = DDR_XFER_SAMPLE_COUNT;
        buffer_to_scale = completed_buffer;
    }
    TRACE_DURATION(T_RX_WORK, DEFAULT_THREAD_ID, t2);
    if (dec->samples_produced >= DDR_XFER_SAMPLE_COUNT) {
        rhf_rhf_rsp_vMultiSclr_asm(dest, buffer_to_scale, &downscalefactor[pipe], DDR_XFER_SIZE_BYTES / DMEM_LINE_SIZE_BYTES);
        if (dmac_is_complete(1 << ddr->dma_channel))
            rx_ddr_dma_complete(pipe);
        rx_lane_try_ddr_enqueue(pipe, dest, rx->buf_timestamp);
        decimate_output_consume(dec);
    }

    // ADC self perpetuating, reenque new tranfer on each completion
    const axiq_config_t *const axiq = afe->config;
    dmac_enable(axiq->dmac_ctrl_flags | DMAC_TRIG_VCPU | DMAC_PEND_EXT, // flags
                ADC_XFER_SIZE_BYTES, // size
                axiq->axi_fifo_addr, // axi addr
                VCPU_ADDR_FOR_DMA(completed_buffer) // dmem addr
    );
    TRACE_DMA_BEGIN(afe_dma_channel(afe), afe->count_dmac_enque & 1);
    ++rx_stats[pipe].afe_enq;
    ++afe->count_dmac_enque;
    TRACE_DURATION(T_ADC_COMPLETE, DEFAULT_THREAD_ID, t1);
}

void rx_ddr_dma_complete(uint16_t pipe) {
    TRACE_START_DURATION(t1);
    ddr_pipeline_t *const ddr = &rx_ddr[pipe];
    TRACE_DMA_END(ddr->dma_channel, ddr->count_dmac_complete & 1);
    const meta_t *const meta = ddr_next_complete_meta(ddr);

    ++ddr->count_dmac_complete;
    ++rx_stats[pipe].dfe_compl;

    dmac_clear_complete(1 << ddr->dma_channel);
    dmac_clear_event(1 << ddr->dma_channel);

    if (meta->flags & IQSTREAM_DMA_TCD_END) {
        rx_wip_tcd_finish(pipe, 0);
        // rx_publish_tcd_status(ddr, &meta->status);
    }

    TRACE_DURATION(T_DDR_WR_COMPLETE, DEFAULT_THREAD_ID, t1);
}

void receiver_init(void) {
    for (uint16_t i = 0; i < RX_PIPE_COUNT; ++i) {
        rx_select_channel(i, (e_rx_channel)(VSPA_RX0 + i));
        rx_set_sample_format(i, IQ_SAMPLE_FORMAT_I16);
    }
}

void rx_lane_prime(uint16_t pipe) {
    adc_pipeline_t *const adc = &rx_adc[pipe];
    const axiq_config_t *const axiq = rx_adc[pipe].afe.config;
    timer_trig_immediate_async(axiq->phytimer_id, ePhyTimerComparatorOut0);

    rx_adc_pipe_reset(adc);
    rx_ddr_pipe_reset(pipe, ddr_write_buffer[pipe]);
    memset16(&rx_stats[pipe], 0, sizeof(struct PipeStats));
    decimation_clear(&dec_pipe[pipe]);
}

// Stream start raises all Rx lanes rx_allowed at stream_origin_phyticks
void rx_stream_started(void) {
    for (int pipe = 0; pipe < RX_PIPE_COUNT; ++pipe)
        rx_adc[pipe].sampling = true;
}

void rx_lane_stop(uint16_t pipe) {
    afe_pipe_abort(&rx_adc[pipe].afe);
    rx_adc[pipe].sampling = false;
}

extern void ProcessTxDMA(void);

// Host reads the combined value of nco_hann_combine()
static nco_correlator_t hann_terms[NCO_HANN_TERMS];
cfloat32_t hann_sum;

cfloat32_t *rx_dft_measure(e_rx_channel channel, int32_t fcw) {
    uint16_t pipe = 0;
    tx_tone_flush(0); // don't measure samples generated before the latest Tx QEC/tone change
    rx_select_channel(pipe, channel);

    ProcessTxDMA(); // for Tx tone generation
    rx_lane_prime(pipe);
    ProcessTxDMA(); // for Tx tone generation
    afe_pipeline_t *const afe = &rx_adc[pipe].afe;
    const axiq_config_t *const axiq = afe->config;
    initial_adc_enq(afe);
    timer_trig_immediate_async(axiq->phytimer_id, ePhyTimerComparatorOut1); // rx_allowed of the measured channel

    const uint16_t fft_size = ADC_XFER_SAMPLE_COUNT;
    const uint16_t dma_mask = (1 << axiq->dma_channel);

    ProcessTxDMA(); // for Tx tone generation
    const uint16_t samples_per_loop = ADC_XFER_SAMPLE_COUNT;

    fcw >>= rx_channel_cfg[channel].oversample_exponent; // adjust for host side expected sample rate
    const uint32_t hann_bin_fcw = (0x80000000u / (fft_size >> 1)); // frequency offset of 1 DFT bin
    memset16(hann_terms, 0, sizeof(hann_terms));

    for (uint16_t i = 0; i < fft_size / samples_per_loop; ++i) {
        volatile int16_t timeout = 2000;
        while (!dmac_is_complete(dma_mask) && --timeout) {
            ProcessTxDMA();
        }
        if (timeout == 0)
            break;

        axiq_fifo_errors(afe);
        dmac_clear_complete(dma_mask);

        cfixed16_t *const completed_buffer = afe_next_complete_buffer(afe);
        ++afe->count_dmac_complete;

        // work
        rx_qec_correction(rx_adc[pipe].channel, completed_buffer, completed_buffer, ADC_DMEM_LINES_COUNT);
        nco_correlate(completed_buffer, ADC_DMEM_LINES_COUNT, fcw, &hann_terms[0]);
        nco_correlate(completed_buffer, ADC_DMEM_LINES_COUNT, fcw - hann_bin_fcw, &hann_terms[1]);
        nco_correlate(completed_buffer, ADC_DMEM_LINES_COUNT, fcw + hann_bin_fcw, &hann_terms[2]);

        const axiq_config_t *const axiq = afe->config;
        dmac_enable(axiq->dmac_ctrl_flags | DMAC_PEND_EXT, // flags
                    ADC_XFER_SIZE_BYTES, // size
                    axiq->axi_fifo_addr, // axi addr
                    VCPU_ADDR_FOR_DMA(completed_buffer) // dmem addr
        );
        ++afe->count_dmac_enque;
    }
    rx_lane_stop(pipe);

    nco_hann_combine(hann_terms, &hann_sum);

    return &hann_sum;
}

// On first TCD enqueues initial DMA transfers for further self triggering
void host_submitted_rx_tcd(uint16_t pipe) {
    afe_pipeline_t *const afe = &rx_adc[pipe].afe;

    if (!tcd_is_valid(&rx_wip_tcd[pipe]) && !tcd_fifo_isempty(&rx_hdma[pipe].tcd_table))
        rx_wip_tcd_load(pipe);

    if (afe_buffers_in_flight(afe) >= MAX_DMA_ENQ)
        return; // skip, all adc buffers busy

    // If rx_allowed is already active, FIFO would start receiving samples at unknown time.
    if (rx_adc[pipe].sampling && (int32_t)(phytime_now() - stream_origin_phyticks) >= 0)
        rx_resync(pipe);
    else
        initial_adc_enq(afe);
}