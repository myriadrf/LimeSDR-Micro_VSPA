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
#include "phytimer.h"
#include "proxy_data.h"

#include "axiq-la9310.h"
#include "vcpu.h"

#include "ditfft.h"

#include "opstatus.h"

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

// How far in the future sampling restart is scheduled, doubled on each failed attempt
#define RX_RESYNC_LEAD_TICKS 1024
#define RX_RESYNC_MAX_ATTEMPTS 6

// when DAC clock divider is disabled, 1 PHYtimer tick can represent only >=2 samples period
static const uint16_t adc_clock_divider_disabled[RX_MAX_LANE_COUNT] = { 0 }; // TODO: read from config registers

extern uint32_t stream_origin_phyticks;
static float
    downscalefactor[RX_MAX_LANE_COUNT]; // when user sample format is 12bit fullscale, downscale from ADC 16bit fullscale to 12bit

cfixed16_t adc_buffer[RX_MAX_LANE_COUNT][MAX_DMA_ENQ * ADC_XFER_SAMPLE_COUNT]
    __attribute__((section(".ippu_dmem"))) _VSPA_VECTOR_ALIGN;
cfixed16_t ddr_write_buffer[RX_MAX_LANE_COUNT][MAX_DMA_ENQ * DDR_XFER_SAMPLE_COUNT]
    __attribute__((section(".vcpu_dmem"))) _VSPA_VECTOR_ALIGN;

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

rx_ddr_pipeline_t rx_ddr[RX_MAX_LANE_COUNT];
adc_pipeline_t rx_adc[RX_MAX_LANE_COUNT];

static inline void memset(void *ptr, uint16_t value, int16_t length) {
    for (int i = 0; i < length; ++i)
        ((uint16_t *)ptr)[i] = value;
}

static inline uint32_t phytime_now(void) { return gpird(4); }

// Converts ADC sample time (phytime ticks since stream origin) into user facing timestamp (samples count after decimation)
static inline uint64_t phyticks_to_ts(uint64_t phyticks, uint16_t lane) {
    return (phyticks << adc_clock_divider_disabled[lane]) >> rx_ddr[lane].decimate_pow2;
}

static inline cfixed16_t *adc_next_complete_buffer(adc_pipeline_t *pipe) {
    return pipe->base_buffer + (pipe->count_dmac_complete & 0x1) * ADC_XFER_SAMPLE_COUNT;
}

static inline uint16_t adc_buffers_in_use(const adc_pipeline_t *pipe) { return pipe->count_dmac_enque - pipe->count_dmac_complete; }

static inline cfixed16_t *ddr_next_enq_buffer(const rx_ddr_pipeline_t *pipe) {
    return pipe->base_buffer + (pipe->count_dmac_enque & 0x1) * DDR_XFER_SAMPLE_COUNT;
}
static inline rx_meta_t *ddr_next_enq_meta(rx_ddr_pipeline_t *pipe) { return &pipe->meta[pipe->count_dmac_enque & 0x1]; }

static inline rx_meta_t *ddr_next_complete_meta(rx_ddr_pipeline_t *pipe) { return &pipe->meta[pipe->count_dmac_complete & 0x1]; }

// @return true if AXIQ FIFO has overrun, and samples were lost
static inline bool check_adc_axiq_errors(uint16_t lane) {
    const enum axiq_fifo_e fifo_index = (enum axiq_fifo_e)rx_adc[lane].axi_fifo_index;
    const uint8_t field_shift = axiq_sr_shift(fifo_index);

    // Check AXIQ rx fifo is not full or overrun
    uint32_t status = axiq_fifo_rx_sr(AXIQ_BANK_0, fifo_index, AXIQ_SR_FIELD_ERROVER | AXIQ_SR_FIELD_ERRUNDER);
    if (status == 0)
        return false;

    status >>= field_shift;
    if (status & AXIQ_SR_FIELD_ERROVER) {
        ++rx_stats[lane].afe_ovr;
        ++proxy.rx_ovr;
        proxy_dirty = true;
    }
    if (status & AXIQ_SR_FIELD_ERRUNDER) {
        ++rx_stats[lane].afe_udr;
    }
    axiq_fifo_rx_cr(AXIQ_BANK_0, fifo_index, AXIQ_CR_CLRERR, AXIQ_CR_CLRERR);
    axiq_fifo_rx_cr(AXIQ_BANK_0, fifo_index, AXIQ_CR_CLRERR, 0);
    return status & AXIQ_SR_FIELD_ERROVER;
}

static inline void rx_wip_tcd_load(rx_ddr_pipeline_t *ddr) {
    ddr->wip_tcd = *tcd_fifo_front(&ddr->dma.tcd_table);
    ddr->wip_tcd_output = tcd_fifo_back(&ddr->output_blocks);
    dma_tcd_reset(ddr->wip_tcd_output);
}

// Completes TCD that is currently being filled. Status is published to host once the last
// data transfer into the TCD buffer completes.
static void rx_wip_tcd_finish(rx_ddr_pipeline_t *ddr, uint32_t extra_flags) {
    // timestamp and size are filled in while the data transfers are enqueued
    ddr->wip_tcd_output->flags |= extra_flags;
    tcd_fifo_push(&ddr->output_blocks); // mark output as ready

    // recirculate TCD to the back of the queue, host buffers are reused continuously
    *(tcd_fifo_back(&ddr->dma.tcd_table)) = *tcd_fifo_front(&ddr->dma.tcd_table);
    tcd_fifo_pop(&ddr->dma.tcd_table);
    tcd_fifo_push(&ddr->dma.tcd_table);
    rx_wip_tcd_load(ddr);

    // Mark input TCD as completed
    ++ddr->dma.tcd_table.done;
    ++proxy.rx_dma_done;
    proxy_dirty = true;
    vspa_to_host_signal(ddr->dma.vth_tcd_done_flag_mask);
}

// Samples following the ones already written are lost. TCD buffer must contain only contiguous samples,
// so partially filled TCD is completed early.
static void rx_wip_tcd_discontinuity(rx_ddr_pipeline_t *ddr) {
    if (tcd_is_valid(&ddr->wip_tcd))
        rx_wip_tcd_finish(ddr, IQSTREAM_TRUNCATED);
}

static void adc_axiq_fifo_reset(uint16_t lane) {
    adc_pipeline_t *const adc = &rx_adc[lane];
    axiq_fifo_rx_enable(AXIQ_BANK_0, (enum axiq_fifo_e)adc->axi_fifo_index);
    const uint32_t dma_mask = (1 << adc->dma_channel);
    dmac_abort(dma_mask);

    axiq_fifo_rx_disable(AXIQ_BANK_0, (enum axiq_fifo_e)adc->axi_fifo_index); // falling edge enters flush mode
    WAIT_FOR(!dmac_is_running(dma_mask), VSPA_DEFAULT_TIMEOUT);

    // fifo ptr_rst, exits flush mode
    const uint32_t ctrl = DMAC_FIFO_RESET | DMAC_RDC | adc->dma_channel;
    dmac_enable(ctrl, 16, adc->axi_fifo_addr, VCPU_ADDR_FOR_DMA(adc_buffer[lane]));

    WAIT_FOR(dmac_is_complete(dma_mask), VSPA_DEFAULT_TIMEOUT);
    dma_clear_all_bits(dma_mask);
}

static inline void rx_adc_pipe_reset(adc_pipeline_t *adc, cfixed16_t *buffer) {
    adc->base_buffer = buffer;
    adc->count_dmac_enque = 0;
    adc->count_dmac_complete = 0;
    adc->phyticks = 0;
    adc->sampling = false;

    // reset dma
    dma_clear_all_bits(1 << adc->dma_channel);
}

static inline void rx_ddr_pipe_reset(rx_ddr_pipeline_t *ddr, cfixed16_t *buffer) {
    ddr->base_buffer = buffer;
    ddr->count_dmac_enque = 0;
    ddr->count_dmac_complete = 0;
    ddr->buf_filled = 0;
    ddr->buf_timestamp = 0;

    ddr->wip_tcd.size = 0;
    ddr->wip_tcd.flags = 0;

    tcd_fifo_reset(&ddr->dma.tcd_table);
    tcd_fifo_reset(&ddr->output_blocks);
    ddr->wip_tcd_output = tcd_fifo_back(&ddr->output_blocks);
    dma_tcd_reset(ddr->wip_tcd_output);
    memset(ddr->meta, 0, sizeof(ddr->meta));

    const uint32_t dma_mask = (1 << ddr->dma_channel);
    dmac_abort(dma_mask);
    WAIT_FOR(!dmac_is_running(dma_mask), VSPA_DEFAULT_TIMEOUT);
    dma_clear_all_bits(dma_mask);
}

int rx_select_channel(uint16_t lane, e_rx_channel channel) {
    if (lane > RX_MAX_LANE_COUNT)
        return lime_Result_InvalidValue;
    adc_pipeline_t *const adc = &rx_adc[lane];
    adc->axi_fifo_addr = RO0_AXI_FIFO_ADDR + (channel * 0x1000) + addr_beat_offset;
    adc->axi_fifo_index = (enum axiq_fifo_e)(AXIQ_FIFO_RX0 + channel);
    adc->dma_channel = RO0_ADC_RD_DMA_CHANNEL + channel;
    adc->phytimer_id = PHY_TIMER_COMP_CH1_RX_ALLOWED + channel;
    rx_adc_pipe_reset(&rx_adc[lane], adc_buffer[lane]);

    rx_ddr[lane].dma_channel = DDR_WR_DMA_CHANNEL_1 + lane;
    rx_ddr_pipe_reset(&rx_ddr[lane], ddr_write_buffer[lane]);

    rx_generator[lane].amplitude = 0.9;
    rx_generator[lane].phase = 0;
    rx_generator[lane].freq_bin = 1;

    rx_ddr[lane].dma.htv_tcd_pending_flag_mask = (HTV_SIGNAL_RXLANE0_TCD_PENDING << lane);
    rx_ddr[lane].dma.vth_tcd_done_flag_mask = (VTH_SIGNAL_RXLANE0_TCD_DONE << lane);

    clear_htv_signal(rx_ddr[lane].dma.htv_tcd_pending_flag_mask);
    return lime_Result_Success;
}

int rx_set_oversampling(uint16_t lane, uint16_t decimate_pow2) {
    if (lane > RX_MAX_LANE_COUNT)
        return lime_Result_InvalidValue;

    rx_ddr[lane].decimate_pow2 = decimate_pow2;
    return lime_Result_Success;
}

int rx_set_sample_format(uint16_t lane, iq_sample_format_e format) {
    if (lane >= RX_MAX_LANE_COUNT)
        return lime_Result_InvalidValue;
    if (format == IQ_SAMPLE_FORMAT_I12)
        downscalefactor[lane] = 0.0625f;
    else
        downscalefactor[lane] = 1.0f;
    return lime_Result_Success;
}

// Prime ADC AXIQ and DMA engine, the actual start is triggered by phytimer
static inline void initial_adc_enq(uint16_t lane) {
    adc_pipeline_t *const adc = &rx_adc[lane];
    const uint16_t dma_mask = 1 << adc->dma_channel;

    WAIT_FOR(!dmac_is_running(1 << adc->dma_channel), VSPA_DEFAULT_TIMEOUT);
    axiq_fifo_rx_enable(AXIQ_BANK_0, (enum axiq_fifo_e)adc->axi_fifo_index);
    // Enabling Rx AXIQ instantly generates underrun error, clear it.
    axiq_fifo_rx_cr(AXIQ_BANK_0, (enum axiq_fifo_e)adc->axi_fifo_index, AXIQ_CR_CLRERR, AXIQ_CR_CLRERR);
    axiq_fifo_rx_cr(AXIQ_BANK_0, (enum axiq_fifo_e)adc->axi_fifo_index, AXIQ_CR_CLRERR, 0);

    // enque two reads
    const uint32_t dma_ctrl = adc->dma_channel | DMAC_FIFO | DMAC_RDC | DMAC_TRIG_VCPU;
    dmac_prep_a_s(ADC_XFER_SIZE_BYTES, adc->axi_fifo_addr);
    dmac_enable_v_c(dma_ctrl, VCPU_ADDR_FOR_DMA(adc->base_buffer));
    TRACE_DMA_BEGIN(adc->dma_channel, rx_stats[lane].afe_enq & 1);
    ++rx_stats[lane].afe_enq;
    ++adc->count_dmac_enque;
    dmac_enable_v_c(dma_ctrl, VCPU_ADDR_FOR_DMA(&adc->base_buffer[ADC_XFER_SAMPLE_COUNT]));
    TRACE_DMA_BEGIN(adc->dma_channel, rx_stats[lane].afe_enq & 1);
    ++rx_stats[lane].afe_enq;
    ++adc->count_dmac_enque;
}

// @param timestamp of the first sample in the buffer
static void rx_lane_try_ddr_enqueue(uint16_t lane, const cfixed16_t *buffer, uint64_t timestamp) {
    TRACE_START_DURATION(t1);
    rx_ddr_pipeline_t *const ddr = &rx_ddr[lane];
    if (!dmac_is_available(1 << ddr->dma_channel)) {
        // should not happen. DDR transfers should complete faster than ADC acquisition.
        // IF ddr is not available, then realtime work is not possible.
        ++rx_stats[lane].dfe_ovr;
        ++proxy.rx_ovr;
        proxy_dirty = true;
        rx_wip_tcd_discontinuity(ddr);
        return;
    }

    if (!tcd_is_valid(&ddr->wip_tcd)) {
        if (tcd_fifo_isempty(&ddr->dma.tcd_table)) {
            ++rx_stats[lane].dfe_udr;
            rx_wip_tcd_discontinuity(ddr);
            return;
        }
        rx_wip_tcd_load(ddr);
    }

    volatile dma_tcd_t *const tcd = &ddr->wip_tcd;
    const uint32_t xfer_size = tcd->size > DDR_XFER_SIZE_BYTES ? DDR_XFER_SIZE_BYTES : tcd->size;

    iowr(DMA_DMEM_PRAM_ADDR, VCPU_ADDR_FOR_DMA(buffer));
    iowr(DMA_AXI_ADDRESS, tcd->la9310_mem_address);
    iowr(DMA_AXI_BYTE_CNT, xfer_size);

    if (ddr->wip_tcd_output->size == 0) {
        // first samples going into the TCD buffer
        ddr->wip_tcd_output->la9310_mem_address = tcd->la9310_mem_address;
        ddr->wip_tcd_output->timestamp_lsb = (uint32_t)timestamp;
        ddr->wip_tcd_output->timestamp_msb = (uint32_t)(timestamp >> 32);
    }

    tcd->la9310_mem_address += xfer_size;
    tcd->size -= xfer_size;
    ddr->wip_tcd_output->size += xfer_size;

    rx_meta_t *const ddr_meta = ddr_next_enq_meta(ddr);
    ddr_meta->flags = tcd->flags;

    // DDR writes complete faster than ADC reads, so no need for DMAC_TRIG_VCPU
    // VCPU will be triggered only by ADC transfers to maintain consistent pacing
    const uint32_t dma_ctrl = DMAC_WRC | ddr->dma_channel;
    iowr(DMA_XFR_CTRL, dma_ctrl);
    TRACE_DMA_BEGIN(ddr->dma_channel, ddr->count_dmac_enque & 1);
    ++ddr->count_dmac_enque;
    rx_stats[lane].dfe_enq = ddr->count_dmac_enque;

    if (tcd->size == 0) {
        ddr_meta->flags |= IQSTREAM_DMA_TCD_END;
    }

    TRACE_DURATION(T_DDR_WR_ENQ, DEFAULT_THREAD_ID, t1);
}

// Restarts sampling at a known phytime to reestablish samples timing. Samples that are still in the
// ADC path are dropped, already sent to DDR remain valid, as they were all received before the overrun.
static void rx_resync(uint16_t lane) {
    TRACE_START_DURATION(t1);
    adc_pipeline_t *const adc = &rx_adc[lane];
    rx_ddr_pipeline_t *const ddr = &rx_ddr[lane];

    // partially accumulated DDR block is dropped, filter history is not contiguous with restarted samples
    ddr->buf_filled = 0;
    memset(decimation_history[lane], 0, sizeof(decimation_history[lane]));
    rx_wip_tcd_discontinuity(ddr);

    // keep restarted samples on the decimated samples grid
    const uint32_t align_mask = (1 << ddr->decimate_pow2) - 1;
    // ADC sample time of the restart, if no samples would have been lost
    const uint32_t expected_phytime = stream_origin_phyticks + (uint32_t)adc->phyticks;

    uint32_t start_phytime;
    uint32_t lead = RX_RESYNC_LEAD_TICKS;
    for (uint16_t attempt = 0;; ++attempt, lead <<= 1) {
        timer_trig_immediate(adc->phytimer_id, ePhyTimerComparatorOut0); // stop samples entering FIFO
        adc_axiq_fifo_reset(lane);
        rx_adc_pipe_reset(adc, adc_buffer[lane]);
        initial_adc_enq(lane);

        if (attempt == RX_RESYNC_MAX_ATTEMPTS) {
            // Could not schedule in time, start immediately and estimate the time
            const uint32_t before_phytime = phytime_now();
            timer_trig_immediate(adc->phytimer_id, ePhyTimerComparatorOut1);
            start_phytime = before_phytime + ((phytime_now() - before_phytime) >> 1);
            break;
        }

        const uint32_t start_offset = ((phytime_now() + lead - stream_origin_phyticks) + align_mask) & ~align_mask;
        start_phytime = stream_origin_phyticks + start_offset;
        timer_trig_schedule(adc->phytimer_id, ePhyTimerComparatorOut1, start_phytime);

        // Comparator triggers only on equality, if start time has already passed it might have been missed.
        if ((int32_t)(phytime_now() - start_phytime) < 0)
            break;
    }

    adc->phyticks += (int32_t)(start_phytime - expected_phytime);
    adc->sampling = true;
    ++rx_stats[lane].afe_drop;
    TRACE_DURATION(T_ADC_AXIQ_RST, DEFAULT_THREAD_ID, t1);
}

static inline void decimate(uint16_t lane, cfixed16_t *restrict dest, volatile cfixed16_t *restrict src, uint16_t src_count) {
    switch (rx_ddr[lane].decimate_pow2) {
    default:
    case 1:
        decimator_2x_8_Taps_asm(dest, src, rx_filter_taps_downsampling, decimation_history[lane], src_count);
        break;
    case 2:
        decimator_4x_8_Taps_asm(dest, src, rx_filter_taps_downsampling, decimation_history[lane], src_count);
        break;
    }
}

void adc_dma_complete(uint16_t lane) {
    adc_pipeline_t *const adc = &rx_adc[lane];
    TRACE_START_DURATION(t1);
    TRACE_DMA_END(adc->dma_channel, adc->count_dmac_complete & 1);

    const uint16_t dma_mask = (1 << adc->dma_channel);
    dmac_clear_complete(dma_mask); // don't care about complete, checking only event
    dmac_clear_event(dma_mask);

    if (check_adc_axiq_errors(lane)) {
        // Unknown amount of samples were lost, completed buffer might already contain the gap.
        rx_resync(lane);
        TRACE_DURATION(T_ADC_COMPLETE, DEFAULT_THREAD_ID, t1);
        return;
    }

    cfixed16_t *const completed_buffer = adc_next_complete_buffer(adc);
    ++adc->count_dmac_complete;
    ++rx_stats[lane].afe_compl;

    const uint64_t buffer_time = adc->phyticks;
    adc->phyticks += (ADC_XFER_SAMPLE_COUNT >> adc_clock_divider_disabled[lane]);

    // work
    rx_ddr_pipeline_t *const ddr = &rx_ddr[lane];
    cfixed16_t *const dest = ddr_next_enq_buffer(ddr);
    TRACE_START_DURATION(t2);
    if (ddr->decimate_pow2) {
        if (ddr->buf_filled == 0)
            ddr->buf_timestamp = phyticks_to_ts(buffer_time, lane);
        // in place processing
        rx_qec_correction(completed_buffer, completed_buffer, ADC_XFER_SAMPLE_COUNT);
        rhf_rhf_rsp_vMultiSclr_asm(completed_buffer, completed_buffer, &downscalefactor[lane],
                                   ADC_XFER_SIZE_BYTES / DMEM_LINE_SIZE_BYTES);
        decimate(lane, dest + ddr->buf_filled, completed_buffer, ADC_XFER_SAMPLE_COUNT);
        TRACE_DURATION(T_RX_WORK, DEFAULT_THREAD_ID, t2);
        ddr->buf_filled += (ADC_XFER_SAMPLE_COUNT >> ddr->decimate_pow2);
        if (ddr->buf_filled >= DDR_XFER_SAMPLE_COUNT) {
            if (dmac_is_complete(1 << ddr->dma_channel))
                rx_ddr_dma_complete(lane);
            rx_lane_try_ddr_enqueue(lane, dest, ddr->buf_timestamp);
            ddr->buf_filled = 0;
        }
    } else {
        // process into ddr buffer
        rx_qec_correction(dest, completed_buffer, ADC_XFER_SAMPLE_COUNT);
        rhf_rhf_rsp_vMultiSclr_asm(dest, dest, &downscalefactor[lane], ADC_XFER_SIZE_BYTES / DMEM_LINE_SIZE_BYTES);
        TRACE_DURATION(T_RX_WORK, DEFAULT_THREAD_ID, t2);
        if (dmac_is_complete(1 << ddr->dma_channel))
            rx_ddr_dma_complete(lane);
        rx_lane_try_ddr_enqueue(lane, dest, phyticks_to_ts(buffer_time, lane));
    }

    // ADC self perpetuating, reenque new tranfer on each completion
    dmac_enable((DMAC_RDC | DMAC_FIFO | DMAC_TRIG_VCPU) | adc->dma_channel, // flags
                ADC_XFER_SIZE_BYTES, // size
                adc->axi_fifo_addr, // axi addr
                VCPU_ADDR_FOR_DMA(completed_buffer) // dmem addr
    );
    TRACE_DMA_BEGIN(adc->dma_channel, adc->count_dmac_enque & 1);
    ++rx_stats[lane].afe_enq;
    ++adc->count_dmac_enque;
    TRACE_DURATION(T_ADC_COMPLETE, DEFAULT_THREAD_ID, t1);
}

void rx_ddr_dma_complete(uint16_t lane) {
    TRACE_START_DURATION(t1);
    rx_ddr_pipeline_t *const ddr = &rx_ddr[lane];
    TRACE_DMA_END(ddr->dma_channel, ddr->count_dmac_complete & 1);
    const rx_meta_t *const meta = ddr_next_complete_meta(ddr);

    ++ddr->count_dmac_complete;
    ++rx_stats[lane].dfe_compl;

    dmac_clear_complete(1 << ddr->dma_channel);
    dmac_clear_event(1 << ddr->dma_channel);

    if (meta->flags & IQSTREAM_DMA_TCD_END) {
        rx_wip_tcd_finish(ddr, 0);
        // rx_publish_tcd_status(ddr, &meta->status);
    }

    TRACE_DURATION(T_DDR_WR_COMPLETE, DEFAULT_THREAD_ID, t1);
}

void receiver_init(void) {
    for (int i = 0; i < RX_MAX_LANE_COUNT; ++i) {
        rx_select_channel(i, (e_rx_channel)(VSPA_RX0 + i));
        rx_set_oversampling(i, 0);
        rx_set_sample_format(i, IQ_SAMPLE_FORMAT_I16);
    }
}

void rx_lane_prime(uint16_t lane) {
    adc_pipeline_t *const adc = &rx_adc[lane];
    timer_trig_immediate(adc->phytimer_id, ePhyTimerComparatorOut0);

    // clear AXIQ errors
    const enum axiq_fifo_e fifo_index = (enum axiq_fifo_e)adc->axi_fifo_index;
    axiq_fifo_rx_cr(AXIQ_BANK_0, fifo_index, AXIQ_CR_CLRERR, AXIQ_CR_CLRERR);
    axiq_fifo_rx_cr(AXIQ_BANK_0, fifo_index, AXIQ_CR_CLRERR, 0);

    rx_adc_pipe_reset(&rx_adc[lane], adc_buffer[lane]);
    rx_ddr_pipe_reset(&rx_ddr[lane], ddr_write_buffer[lane]);
    memset(&rx_stats[lane], 0, sizeof(struct PipeStats));
    memset(decimation_history[lane], 0, sizeof(decimation_history[lane]));

    adc_axiq_fifo_reset(lane);
}

// Stream start raises all Rx lanes rx_allowed at stream_origin_phyticks
void rx_stream_started(void) {
    for (int lane = 0; lane < RX_MAX_LANE_COUNT; ++lane)
        rx_adc[lane].sampling = true;
}

void rx_lane_stop(uint16_t lane) {
    const uint32_t dma_mask = (1 << rx_ddr[lane].dma_channel) | (1 << rx_adc[lane].dma_channel);
    axiq_fifo_rx_disable(AXIQ_BANK_0, (enum axiq_fifo_e)rx_adc[lane].axi_fifo_index); // enter DMA flush mode
    dmac_abort(dma_mask);
    // WAIT_FOR(dmac_is_available(1 << rx_adc[lane].dma_channel), VSPA_DEFAULT_TIMEOUT);
    // stream_read_ptr_rst(lane);
    WAIT_FOR(!dmac_is_running(dma_mask), VSPA_DEFAULT_TIMEOUT);
    timer_trig_immediate(rx_adc[lane].phytimer_id, ePhyTimerComparatorOut0);
    rx_adc[lane].sampling = false;
    dmac_clear_complete(dma_mask);
    dmac_clear_event(dma_mask);
}

vspa_complex_float32 *rx_fft(uint16_t channel) {
    return 0;
    /*
    uint16_t lane = 0;
    rx_select_channel(lane, channel);

    timer_trig_immediate(3, ePhyTimerComparatorOut1);
    rx_lane_prime(lane);
    rx_ddr_pipeline_t *const ddr = &rx_ddr[lane];
    cfixed16_t *const fft_input = ddr->base_buffer;

    const uint16_t fft_size = DDR_XFER_SAMPLE_COUNT;
    const uint16_t dma_mask = (1 << rx_adc[lane].dma_channel);

    for (uint16_t i = 0; i < fft_size / ADC_XFER_SAMPLE_COUNT; ++i) {
        WAIT_FOR(dmac_is_complete(dma_mask), VSPA_DEFAULT_TIMEOUT);
        check_adc_axiq_errors(lane);
        dmac_clear_complete(dma_mask);

        ++rx_stats[lane].afe_compl;

        cfixed16_t *const completed_buffer = rx_adc[lane].next_completion_buffer;
        cfixed16_t *const dest = fft_input + ddr->buf_filled;

        // work
        const uint16_t input_count = ADC_XFER_SAMPLE_COUNT;
        if (ddr->decimate_pow2) {
            // in place processing
            rx_qec_correction(completed_buffer, completed_buffer, ADC_XFER_SAMPLE_COUNT);
            decimate(lane, dest, completed_buffer, ADC_XFER_SAMPLE_COUNT);
        } else {
            // process into ddr buffer
            rx_qec_correction(dest, completed_buffer, ADC_XFER_SAMPLE_COUNT);
        }
        ddr->buf_filled += (input_count >> ddr->decimate_pow2);

        // enque new tranfer
        if (dmac_is_available(dma_mask)) {
            dmac_enable(rx_adc[lane].dma_channel | DMAC_RDC | DMAC_FIFO | DMAC_TRIG_VCPU, // flags
                        ADC_XFER_SIZE_BYTES, // size
                        rx_adc[lane].axi_fifo_addr, // axi addr
                        VCPU_ADDR_FOR_DMA(completed_buffer) // dmem addr
            );
            ++rx_stats[lane].afe_enq;
        } else
            ++rx_stats[lane].afe_ovr;

        ++rx_adc[lane].count_dmac_complete;
        rx_adc[lane].next_completion_buffer = rx_adc[lane].base_buffer + (rx_adc[lane].count_dmac_complete & 0x1) *
    ADC_XFER_SAMPLE_COUNT;
    }
    timer_trig_immediate(3, ePhyTimerComparatorOut0);
    rx_lane_stop(lane);

    // memclr(adc_buffer[lane], ADC_XFER_SAMPLE_COUNT*2);
    vspa_complex_float32 *fft_output = (vspa_complex_float32 *)adc_buffer[lane];
    fftDIF512_hfx_sfl(fft_input, fft_output, fft_input, DDR_XFER_SAMPLE_COUNT * 2);
    return (vspa_complex_float32 *)fft_output;*/
}

extern void ProcessTxDMA(void);

cfixed16_t *capture_adc(uint16_t channel) {
    uint16_t lane = 0;
    rx_select_channel(lane, (e_rx_channel)channel);

    rx_lane_prime(lane);
    initial_adc_enq(lane);
    timer_trig_immediate(3, ePhyTimerComparatorOut1);
    rx_ddr_pipeline_t *const ddr = &rx_ddr[lane];

    const uint16_t fft_size = DDR_XFER_SAMPLE_COUNT;
    const uint16_t dma_mask = (1 << rx_adc[lane].dma_channel);

    ProcessTxDMA(); // for Tx tone generation
    const uint16_t samples_per_loop = ADC_XFER_SAMPLE_COUNT >> ddr->decimate_pow2;
    cfixed16_t *const dest = ddr->base_buffer;

    for (uint16_t i = 0; i < fft_size / samples_per_loop; ++i) {
        int16_t timeout = 2000;
        while (!dmac_is_complete(dma_mask) && --timeout) {
            ProcessTxDMA();
        }
        if (timeout == 0)
            break;

        check_adc_axiq_errors(lane);
        dmac_clear_complete(dma_mask);

        ++rx_stats[lane].afe_compl;

        cfixed16_t *const completed_buffer =
            rx_adc[lane].base_buffer + (rx_adc[lane].count_dmac_complete & 0x1) * ADC_XFER_SAMPLE_COUNT;

        // work
        const uint16_t input_count = ADC_XFER_SAMPLE_COUNT;
        if (ddr->decimate_pow2) {
            // in place processing
            rx_qec_correction(completed_buffer, completed_buffer, ADC_XFER_SAMPLE_COUNT);
            decimate(lane, dest + ddr->buf_filled, completed_buffer, ADC_XFER_SAMPLE_COUNT);
        } else {
            rx_qec_correction(dest + ddr->buf_filled, completed_buffer, ADC_XFER_SAMPLE_COUNT);
        }
        // ddr->buf_filled += (ADC_XFER_SAMPLE_COUNT >> ddr->decimate_pow2);

        dmac_enable(rx_adc[lane].dma_channel | DMAC_RDC | DMAC_FIFO, // flags
                    ADC_XFER_SIZE_BYTES, // size
                    rx_adc[lane].axi_fifo_addr, // axi addr
                    VCPU_ADDR_FOR_DMA(completed_buffer) // dmem addr
        );
        ++rx_stats[lane].afe_enq;
        ++rx_adc[lane].count_dmac_enque;
        ++rx_adc[lane].count_dmac_complete;
    }
    timer_trig_immediate(3, ePhyTimerComparatorOut0);
    rx_lane_stop(lane);

    return ddr->base_buffer;
}

// On first TCD enqueues initial DMA transfers for further self triggering
void host_submitted_rx_tcd(uint16_t lane) {
    rx_ddr_pipeline_t *const ddr = &rx_ddr[lane];
    if (!tcd_is_valid(&ddr->wip_tcd) && !tcd_fifo_isempty(&ddr->dma.tcd_table))
        rx_wip_tcd_load(ddr);

    if (adc_buffers_in_use(&rx_adc[lane]) >= MAX_DMA_ENQ)
        return; // skip, all adc buffers busy

    // If rx_allowed is already active, FIFO would start receiving samples at unknown time.
    if (rx_adc[lane].sampling && (int32_t)(phytime_now() - stream_origin_phyticks) >= 0)
        rx_resync(lane);
    else
        initial_adc_enq(lane);
}