// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#include "dmac.h"
#include "dfe.h"
#include "txiqcomp.h"
#include "l1-trace.h"
#include "transmitter.h"
#include "vspa_state.h"
#include "pipelines.h"
#include "memory_pool.h"
#include "dma_common.h"
#include "platform.h"
#include "vcpu.h"
#include "gpio.h"
#include "ccnt.h"

#include "receiver.h"
#include "tone_generator.h"
#include "timer_control.h"

#include "compiler.h"

#define TX_DMA_ALLOWED_TIMER 11

#define MAX_DMA_ENQUE_COUNT 2
#define TX_DAC_DMA_SAMPLE_COUNT (256)
#define TX_DAC_BUF_COUNT 4 // 2 DAC, 1 DDR, 1 DDR or Interpolation

#define DAC_DMA_STEP (4 * TX_DAC_DMA_SAMPLE_COUNT)
#define DAC_AXI_FIFO_SIZE_BYTES 256

#define TX_DAC_DMA_MASK (0x1 << TX_DAC_WR_DMA_CHANNEL)
#define DDR_RD_DMA_MASK (0x1 << DDR_RD_DMA_CHANNEL_1)
#define DDR_RD_BURST 1

#define TX_CONFIG player_state.info.tx_config
#define TX_CONTROL player_state.internals.tx_control

#define DMA_TABLE player_state.internals.tx_dma_schedule

#define BIT(x) (1 << x)

// due to dma transfers taking time, flush is executed over multiple go events
enum {
    DAC_FLUSH_STAGE_NONE = 0,
    DAC_FLUSH_STAGE_ABORT,
    DAC_FLUSH_STAGE_WAIT_DMA_END,
    DAC_FLUSH_STAGE_FIFO_RST,
    DAC_FLUSH_STAGE_DONE,
};

tx_dma_hif_t tx_dma_interface;
static int tx_flush_stage = 0;

D_STATIC bool wait_tx_burst_end = 0;
D_STATIC bool recovery = 0;
D_STATIC bool fast_forward_ddr = 0;
D_STATIC tone_state_t tx_tone_state;

D_STATIC cfixed16_t dac_buffer[TX_DAC_BUF_COUNT * TX_DAC_DMA_SAMPLE_COUNT] __attribute__((aligned(64), section(".vcpu_dmem")));

static MemoryBlock_t meta_blocks[TX_DAC_BUF_COUNT];
static D_STATIC HandlesStack_t handles_pool;

#define TX_DAC_FIFO_BEAT_COUNT 16
#define dac_axi_fifo_addr (0x4400B000 + 0x1000 - TX_DAC_FIFO_BEAT_COUNT * 16) // axi_DAC_FIFO_addr

D_STATIC struct {
    dma_line_t batch;
    struct flow_control flow;
} ddr_src;

static void dac_enqueue(tx_pipeline_t *pipe);

static void ddr_src_reset(void) {
    ddr_src.batch.size = 0;
    ddr_src.flow.consumed = 0;
    ddr_src.flow.produced = 0;
}

static inline bool ddr_src_isready(void) { return ddr_src.batch.size > 0; }

inline static bool should_dac_enqueue(tx_pipeline_t *pipe) {
    return (dmac_is_available(TX_DAC_DMA_MASK) && !fifo_isempty(pipe->dac.input.fifo) &&
            fifo_size(pipe->dac.output.fifo) < MAX_DMA_ENQUE_COUNT && !wait_tx_burst_end);
}

#define txpipe player_state.internals.txpipe

static inline uint32_t get_phytimer_counter() { return gpird(4); }

static inline void TxAXIQ(bool enable) // for tracing pursposes
{
    TRACE_COUNTER(CNT_TX_AXIQ_EN, enable);
    if (enable) {
        axiq_tx_enable();
    } else {
        axiq_tx_disable();
    }
}

// PTR_RST must be done after:
// TX_DMA_Allowed(PHYTimer11) falling edge
// TX_AXIQ_Disable(), entered flush mode
static void stream_write_ptr_rst(uint32_t dma_channel_wr, uint32_t axi_wr) {
    const uint32_t ctrl = DMAC_FIFO_RESET | DMAC_WRC | dma_channel_wr;
    const uint32_t vsp = VSPA_HALF_WORDS(dac_buffer);
    // while (!dmac_is_available(BIT(dma_channel_wr))) {
    // }
    WAIT_TIMEOUT_R(dmac_is_available(BIT(dma_channel_wr)), 5000);
    dmac_enable(ctrl, 2048, axi_wr, vsp);
}

static inline void stream_write(uint32_t flags, uint32_t axi_wr, uint32_t vsp, uint32_t size_bytes) {
    dmac_enable(flags, size_bytes, axi_wr, vsp);
}

void InitializeTx(void) {
    TX_CONTROL.ddr_enabled = 0;
    TX_CONTROL.generate_tone = 0;
    TX_CONTROL.dma_table_loop = 0;

    TX_CONFIG.oversample = 1;
    // TX_CONFIG.host_fifo_step = DAC_DMA_STEP / TX_CONFIG.oversample;
    // TX_CONFIG.host_fifo_step = DAC_DMA_STEP;

    tx_tone_state.amplitude = 0.9;
    tx_tone_state.phase = 0;
    tx_tone_state.freq_bin = 8192;

    tx_dma_interface.loop_counter = 0;

    TxConfigure(TX_CONFIG.oversample);
    EnqueueProxyUpdate(PROXY_UPDATE_FLOW | PROXY_UPDATE_INFO);
}

#define TX_MAX_UPSAMPLE_TAPS 64
D_STATIC cfixed16_t tx_interpolation_history[TX_MAX_UPSAMPLE_TAPS - 1] __attribute__((aligned(64), section(".vcpu_dmem"))) = { 0 };
// Interpolation function prototypes
void X2_interp_tap32_filter(__fx16 *output, __fx16 *input, unsigned int num_samples, __fx16 *history, float *filter_taps);
void X4_interp_tap64_filter(__fx16 *output, __fx16 *input, unsigned int num_samples, __fx16 *history, float *filter_taps);

D_STATIC const float tx_filter_taps_upsampling_x2[32] __attribute__((aligned(64))) = {
#include "fir_interpolation_x2.txt"
};
// same coefficients as x2, just interleaved multiple instances.
D_STATIC const float tx_filter_taps_upsampling_x4[128] __attribute__((aligned(64))) = {
#include "fir_interpolation_x4.txt"
};

static MetaHandle_t interpol_cache = INVALID_HANDLE;
static uint32_t cache_offset = 0;
static interpol_in_samples_count = TX_DAC_DMA_SAMPLE_COUNT;

static void ddr_enqueue(tx_pipeline_t *pipe);

static inline bool should_ddr_enqueue(tx_pipeline_t *pipe) {
    const int16_t active_buffers = fifo_size(pipe->ddr.input.fifo) + fifo_size(pipe->ddr.output.fifo);
    return dmac_is_available(DDR_RD_DMA_MASK) && fifo_size(pipe->ddr.input.fifo) < 2 && active_buffers <= 2;
}

static inline void interpolate(tx_pipeline_t *pipe) {
    if (fifo_isempty(pipe->interp.input.fifo))
        return;

    TRACE_START_DURATION(t1);

    if (interpol_cache == INVALID_HANDLE) {
        interpol_cache = fifo_front(pipe->interp.input.fifo);
        cache_offset = 0;
        TRACE_COUNTER(CNT_INTERP, cache_offset);
    }

    if (handles_stack_isempty(&handles_pool)) {
        TRACE_EVENT(T_NO_MEMORY, 1, 0);
        TRACE_DURATION(T_INT_BUFFER, 1, t1);
        return;
    }

    MetaHandle_t out_handle = handles_stack_top(&handles_pool);
    handles_stack_pop(&handles_pool);
    TRACE_COUNTER(CNT_POOL, handles_pool.count);

    MemoryBlock_t *src_block = &meta_blocks[interpol_cache];
    MemoryBlock_t *dest_block = &meta_blocks[out_handle];

    dest_block->timestamp = src_block->timestamp + cache_offset * TX_CONFIG.oversample;
    dest_block->flags = src_block->flags;

    if (cache_offset == 0 && (src_block->flags & PKT_START)) {
        memclr(tx_interpolation_history, sizeof(tx_interpolation_history));
        src_block->flags &= ~PKT_START;
    }

    const cfixed16_t *in = ((cfixed16_t *)src_block->addr) + cache_offset;
    {
        TRACE_START_DURATION(t2);
        switch (TX_CONFIG.oversample) {
        case 2:
            X2_interp_tap32_filter((__fx16 *)dest_block->addr, (__fx16 *)in, interpol_in_samples_count,
                                   (__fx16 *)tx_interpolation_history, (float *)tx_filter_taps_upsampling_x2);
            break;
        case 4:
            X4_interp_tap64_filter((__fx16 *)dest_block->addr, (__fx16 *)in, interpol_in_samples_count,
                                   (__fx16 *)tx_interpolation_history, (float *)tx_filter_taps_upsampling_x4);
            break;
        default:
            break;
        }
        TRACE_DURATION(T_INT_BUFFER, 1, t2);
    }
    {
        TRACE_START_DURATION(t2);
        // in place
        tx_qec_correction(dest_block->addr, dest_block->addr, TX_DAC_DMA_SAMPLE_COUNT);
        TRACE_DURATION(T_QEC_TX_BUFFER, 1, t2);
    }

    cache_offset += interpol_in_samples_count;
    TRACE_COUNTER(CNT_INTERP, cache_offset);

    dest_block->flags &= cache_offset < TX_DAC_DMA_SAMPLE_COUNT
                             ? ~PKT_END
                             : dest_block->flags; // clear end mark, if not last of interpolation output
    fifo_push(pipe->interp.output.fifo, out_handle);
    TRACE_COUNTER(CNT_DAC_READY, fifo_size(pipe->interp.output.fifo));

    if (cache_offset == TX_DAC_DMA_SAMPLE_COUNT) {
        fifo_pop(pipe->interp.input.fifo);
        TRACE_COUNTER(CNT_DDR_RD_READY, fifo_size(pipe->interp.input.fifo));

        handles_stack_push(&handles_pool, interpol_cache);
        TRACE_COUNTER(CNT_POOL, handles_pool.count);
        interpol_cache = INVALID_HANDLE;
        cache_offset = 0;
        TRACE_COUNTER(CNT_INTERP, cache_offset);
        if (should_ddr_enqueue(pipe))
            ddr_enqueue(pipe);
    }

    TRACE_DURATION(T_INT_BUFFER, 1, t1);
}

static void InitDAC_pool(HandlesStack_t *pool, cfixed16_t *mem_ptr) {
    handles_stack_clear(pool);
    uint16_t h = 0;
    for (h = 0; h < TX_DAC_BUF_COUNT; ++h) {
        meta_blocks[h].addr = &mem_ptr[TX_DAC_DMA_SAMPLE_COUNT * (TX_DAC_BUF_COUNT - h - 1)];
        meta_blocks[h].size = DAC_DMA_STEP;
        meta_blocks[h].flags = 0;
        meta_blocks[h].timestamp = 0;
        handles_stack_push(pool, h);
        TRACE_COUNTER(CNT_POOL, handles_pool.count);
    }
}

static struct MemoryFIFO ddr_enq_fifo;
static struct MemoryFIFO ddr_ready_fifo;
static struct MemoryFIFO interp_out_fifo;
static struct MemoryFIFO dac_enq_fifo;

static void tx_pipeline_setup(tx_pipeline_t *pipe) {
    stage_setup(&pipe->ddr, &ddr_enq_fifo, &ddr_ready_fifo);
    struct MemoryFIFO *last_stage_fifo = &ddr_ready_fifo;

    stage_setup(&pipe->interp, &ddr_ready_fifo, &interp_out_fifo);
    if (TX_CONFIG.oversample > 1) {
        memclr(tx_interpolation_history, sizeof(tx_interpolation_history));
        last_stage_fifo = &interp_out_fifo;
    }
    stage_setup(&pipe->dac, last_stage_fifo, &dac_enq_fifo);
    InitDAC_pool(&handles_pool, dac_buffer);
}

static void ddelay(uint16_t s) {
    uint32_t volatile cnt;
    for (cnt = s; cnt; --cnt) {
    }
}

static void tx_pipeline_reset(tx_pipeline_t *pipe) {
    interpol_cache = INVALID_HANDLE;
    cache_offset = 0;

    TxAXIQ(true); // enters DMA FLUSH mode
    ddelay(10);
    TxAXIQ(false); // enters DMA FLUSH mode
    dmac_abort(BIT(DDR_RD_DMA_CHANNEL_1));

    uint32_t timer_dma = timer_trig_immediate_async(TX_DMA_ALLOWED_TIMER, ePhyTimerComparatorOut1);

    fifo_reset(&ddr_enq_fifo);
    fifo_reset(&ddr_ready_fifo);
    if (TX_CONFIG.oversample > 1) {
        fifo_reset(&interp_out_fifo);
        // InitDDR_pool(&ddr_pool, tx_ddr_buffer);
        memclr(tx_interpolation_history, sizeof(tx_interpolation_history));
    }
    fifo_reset(&dac_enq_fifo);

    pipe->ddr.input.bytes_done = 0;
    pipe->ddr.output.bytes_done = 0;
    pipe->dac.input.bytes_done = 0;
    pipe->dac.output.bytes_done = 0;
    InitDAC_pool(&handles_pool, dac_buffer);

    const uint32_t dmamask = TX_DAC_DMA_MASK | BIT(DDR_RD_DMA_CHANNEL_1);
    WAIT_TIMEOUT_R(!dmac_is_running(dmamask), 10000);

    wait_for_dma(timer_dma);

    stream_write_ptr_rst(TX_DAC_WR_DMA_CHANNEL, dac_axi_fifo_addr); // must be done to exit DMA FLUSH mode
    WAIT_TIMEOUT_R(!dmac_is_running(dmamask), 10000);
    timer_dma = timer_trig_immediate_async(TX_DMA_ALLOWED_TIMER, ePhyTimerComparatorOut0);

    if (TX_DAC_FIFO_BEAT_COUNT == 16)
        gpowr(7, 0x6, 0x0 << 1);
    else if (TX_DAC_FIFO_BEAT_COUNT == 8)
        gpowr(7, 0x6, 0x1 << 1);
    else if (TX_DAC_FIFO_BEAT_COUNT == 4)
        gpowr(7, 0x6, 0x2 << 1);
    else if (TX_DAC_FIFO_BEAT_COUNT == 2)
        gpowr(7, 0x6, 0x3 << 1);

    TxAXIQ(true);

    dmac_clear_complete(dmamask);
    dmac_clear_event(dmamask);
    dmac_clear_errxfr(dmamask);

    axiq_fifo_tx_cr(AXIQ_BANK_0, AXIQ_FIFO_TX0, AXIQ_CR_CLRERR, AXIQ_CR_CLRERR);
    axiq_fifo_tx_cr(AXIQ_BANK_0, AXIQ_FIFO_TX0, AXIQ_CR_CLRERR, 0);
    wait_for_dma(timer_dma);
}

static void DDR_read_multi_dma(uint32_t DDR_rd_dma_channel, uint32_t nb_dma, uint32_t DDR_address, uint32_t vsp_address,
                               int32_t bytes_size) {
    uint32_t size = bytes_size / nb_dma;

    // user should ensure bytes_size/nb_dma is enire and aligned on AXI bus width 16B.
    // 2048B is ok with 1,2,4 dmas only
    // dma channel should be contiguous

    for (uint32_t i = 0; i < nb_dma; ++i) {
#pragma loop_count(1, 16, 2, 0)
        uint32_t ctrl = DMAC_RDC | (DDR_rd_dma_channel + i) | DMAC_TRIG_VCPU;
#if DDR_RD_BURST
        ctrl |= DMAC_MBRE;
#endif
        dmac_enable(ctrl, size, DDR_address + i * size, vsp_address + i * size);
    }
}

static inline void DDR_read(uint32_t DDR_address, uint32_t vsp_address, uint16_t DDR_rd_dma_channel, uint16_t bytes_size) {
    uint32_t ctrl = DMAC_RDC | DDR_rd_dma_channel | DMAC_TRIG_VCPU;
#if DDR_RD_BURST
    ctrl |= DMAC_MBRE;
#endif
    dmac_enable(ctrl, bytes_size, DDR_address, vsp_address);
}

lime_Result TxConfigure(uint32_t oversample) {
    ddr_src_reset();

    // if (!TX_CONFIG.ddr_rd_dma_ch_nb) {
    //     // LA9310 AXI bus supports 4 opened RD transactions
    //     // Read measurements ( wo/ multi-burst):
    //     // 1x VSPA DMA read    DDR 222 MB/s
    //     // 2x VSPA DMA read    DDR 443 MB/s       <-- default
    //     // 4x VSPA DMA read    DDR 661 MB/s
    //     // Read measurements ( w/ multi-burst):
    //     // 1x VSPA DMA read    DDR 516 MB/s
    //     // 2x VSPA DMA read    DDR 871 MB/s
    //     // 4x VSPA DMA read    DDR 861 MB/s
    //     // 4 channels will cause AXIQ FIFO read starvation if rx is also running
    //     TX_CONTROL.ddr_rd_dma_mBurst = 1;
    //     TX_CONFIG.ddr_rd_dma_ch_nb = 1;
    // }

    TX_CONFIG.oversample = oversample;
    interpol_in_samples_count = TX_DAC_DMA_SAMPLE_COUNT / TX_CONFIG.oversample;

    EnqueueProxyUpdate(PROXY_UPDATE_FLOW | PROXY_UPDATE_INFO);
    return lime_Result_Success;
}

lime_Result TxDDR_control(uint64_t msg64) {
    const bool ddr_enable = (HIWORD(msg64)) & (1 << 0);

    if (TX_CONTROL.ddr_enabled == ddr_enable)
        return lime_Result_Busy; // prevent repeated starts
    // repeated stops are ok

    lime_Result result = lime_Result_Success;
    if (ddr_enable) {
        iowr(EXT_GO_STAT, 0xFF, 0xFF); // clear external GO
        timer_trig_immediate(VSPA_GO_PHYTIMER_ID, ePhyTimerComparatorOut0); // GO triggered only by rising edge

        tx_flush_stage = DAC_FLUSH_STAGE_NONE;
        wait_tx_burst_end = 0;
        recovery = 0;
        fast_forward_ddr = 0;
        memclr((void *)&player_state.data_flow.tx_issues, sizeof(player_state.data_flow.tx_issues));

        TX_CONTROL.ddr_enabled = true;
        tx_pipeline_setup(&txpipe);
        tx_pipeline_reset(&txpipe);
        EnqueueProxyUpdate(PROXY_UPDATE_FLOW | PROXY_UPDATE_INFO);

        dma_table_reset(&DMA_TABLE);
        ddr_src_reset();
    } else {
        TX_CONTROL.ddr_enabled = false;
        timer_trig_immediate(TX_DMA_ALLOWED_TIMER, ePhyTimerComparatorOut0);
        TxAXIQ(false);

        MarkEvent(EVENT_TX_DONE);
        EnqueueProxyUpdate(PROXY_UPDATE_FLOW | PROXY_UPDATE_INFO | PROXY_UPDATE_INTERRUPT);
        ddr_src_reset();
    }

    return result;
}

static inline void TxGenerateTone(void) {
    TRACE_START_DURATION(t1);
    if (handles_stack_isempty(&handles_pool)) {
        TRACE_EVENT(T_NO_MEMORY, DEFAULT_THREAD_ID, 0);
        TRACE_DURATION(T_GENERATE_TONE, DEFAULT_THREAD_ID, t1);
        return;
    }

    const MetaHandle_t handle = handles_stack_top(&handles_pool);
    handles_stack_pop(&handles_pool);
    meta_blocks[handle].flags = 0;

    gen_nco_single_tone(meta_blocks[handle].addr, TX_DAC_DMA_SAMPLE_COUNT, &tx_tone_state);

    TRACE_START_DURATION(t2);
    // in place
    tx_qec_correction(meta_blocks[handle].addr, meta_blocks[handle].addr, TX_DAC_DMA_SAMPLE_COUNT);
    TRACE_DURATION(T_QEC_TX_BUFFER, DEFAULT_THREAD_ID, t2);

    fifo_push(txpipe.dac.input.fifo, handle);

    if (should_dac_enqueue(&txpipe))
        dac_enqueue(&txpipe);
    TRACE_DURATION(T_GENERATE_TONE, DEFAULT_THREAD_ID, t1);
}

void TxTone_control(uint64_t msg64) {
    bool enable = HIWORD(msg64) & 0x00100000;
    // tx_tone_state.amplitude = (LOWORD(msg64) >> 16) / 32768.0;
    tx_tone_state.freq_bin = LOWORD(msg64) & 0xFFFF;
    // tx_tone_state.phase = 0;
    if (enable != TX_CONTROL.generate_tone) {
        if (enable) {
            TRACE_EVENT(T_TIME_NOW, DEFAULT_THREAD_ID, 1);
            memclr((void *)&player_state.data_flow.tx_issues, sizeof(player_state.data_flow.tx_issues));
            tx_pipeline_setup(&txpipe);
            tx_pipeline_reset(&txpipe);

            timer_trig_immediate(TX_DMA_ALLOWED_TIMER, ePhyTimerComparatorOut1);
            for (uint16_t i = 0; i < MAX_DMA_ENQUE_COUNT; ++i)
                TxGenerateTone();
        } else {
            TRACE_EVENT(T_TIME_NOW, DEFAULT_THREAD_ID, 0);
            timer_trig_immediate(TX_DMA_ALLOWED_TIMER, ePhyTimerComparatorOut0);
            TxAXIQ(false);
        }
    }
    TX_CONTROL.generate_tone = enable;
}

static bool check_dac_had_issues() {
    // Check AXIQ tx fifo is not full or overrun
    uint32_t status = axiq_fifo_tx_sr(AXIQ_BANK_0, AXIQ_FIFO_TX0, AXIQ_SR_FIELD_ERROVER | AXIQ_SR_FIELD_ERRUNDER);
    if (status == 0)
        return false;

    const uint8_t field_shift = axiq_sr_shift(AXIQ_FIFO_TX0);
    status >>= field_shift;
    if (status & AXIQ_SR_FIELD_ERROVER) {
        ++player_state.data_flow.tx_issues.overrun;
        TRACE_COUNTER(CNT_TX_OVR, player_state.data_flow.tx_issues.overrun);
    }
    if (status & AXIQ_SR_FIELD_ERRUNDER) {
        ++player_state.data_flow.tx_issues.underrun;
        TRACE_COUNTER(CNT_TX_UDR, player_state.data_flow.tx_issues.underrun);
    }
    EnqueueProxyUpdate(PROXY_UPDATE_FLOW);
    axiq_fifo_tx_cr(AXIQ_BANK_0, AXIQ_FIFO_TX0, AXIQ_CR_CLRERR, AXIQ_CR_CLRERR);
    axiq_fifo_tx_cr(AXIQ_BANK_0, AXIQ_FIFO_TX0, AXIQ_CR_CLRERR, 0);
    return true;
}

static void ddr_completion(tx_pipeline_t *pipe) {
    TRACE_START_DURATION(t1);

    // if (!dmac_is_complete(DDR_RD_DMA_MASK))
    //     return;

    dmac_error_count(DDR_RD_DMA_MASK, &player_state.data_flow.tx_issues.xfer_errors);
    dmac_clear_complete(DDR_RD_DMA_MASK);
    dmac_clear_event(DDR_RD_DMA_MASK);

    const MetaHandle_t handle = fifo_front(pipe->ddr.input.fifo);
    fifo_pop(pipe->ddr.input.fifo);

    // TRACE_EVENT(T_DDR_RD, 1, handle);
    TRACE_COUNTER(CNT_DDR_RD_ENQ, fifo_size(pipe->ddr.input.fifo));
    TRACE_DMA_END(DDR_RD_DMA_CHANNEL_1, handle);
    // if (TX_CONTROL.generate_tone) // replace received data with generated one
    //     gen_nco_single_tone(meta_blocks[handle].addr, TX_DAC_DMA_SAMPLE_COUNT / TX_CONFIG.oversample, &tx_tone_state);

    // player_state.data_flow.tx.produced += TX_CONFIG.host_fifo_step;

    pipe->ddr.output.bytes_done += DAC_DMA_STEP;

    fifo_push(pipe->ddr.output.fifo, handle);
    if (TX_CONFIG.oversample == 1) {
        TRACE_START_DURATION(t2);
        // in place
        tx_qec_correction(meta_blocks[handle].addr, meta_blocks[handle].addr, TX_DAC_DMA_SAMPLE_COUNT);
        TRACE_DURATION(T_QEC_TX_BUFFER, 1, t2);
        TRACE_COUNTER(CNT_DAC_READY, fifo_size(pipe->ddr.output.fifo));
    } else
        TRACE_COUNTER(CNT_DDR_RD_READY, fifo_size(pipe->ddr.output.fifo));

    if (meta_blocks[handle].flags & PKT_IRQ)
        EnqueueProxyUpdate(PROXY_UPDATE_FLOW | PROXY_UPDATE_INTERRUPT);

    if (meta_blocks[handle].flags & PKT_DMA_TCD_END)
        ++tx_dma_interface.loop_counter;

    TRACE_DURATION(T_DDR_COMPLETE, DEFAULT_THREAD_ID, t1);
}

inline static void GetNextDDR(void) {
    if (ddr_src_isready() || dma_table_isempty(&DMA_TABLE))
        return;

    ddr_src.batch = *dma_table_front(&DMA_TABLE);
    ddr_src.flow.consumed = 0;
    ddr_src.flow.produced = 0;
}

static void ddr_enqueue(tx_pipeline_t *pipe) {
    TRACE_START_DURATION(t1);
    GetNextDDR();
    if (!ddr_src_isready())
        return;

    if (handles_stack_isempty(&handles_pool)) {
        TRACE_EVENT(T_NO_MEMORY, 1, 0);
        return;
    }

    const MetaHandle_t handle = handles_stack_top(&handles_pool);
    handles_stack_pop(&handles_pool);
    // TRACE_COUNTER(CNT_POOL, handles_pool.count);

    MemoryBlock_t *dest_block = &meta_blocks[handle];

    const uint32_t srcdata = ((uint32_t)ddr_src.batch.addr) + ddr_src.flow.consumed;

    dest_block->timestamp = ddr_src.batch.timestamp + (ddr_src.flow.consumed / 4) * TX_CONFIG.oversample;
    dest_block->flags = ddr_src.batch.flags;

    if (ddr_src.flow.consumed == 0)
        ddr_src.batch.flags &= ~PKT_START; // clear start flag so that only the first memory chunk would have it

    ddr_src.flow.consumed += DAC_DMA_STEP;

    if (ddr_src.flow.consumed != ddr_src.batch.size)
        dest_block->flags &= ~PKT_END; // clear end flag so that only the last memory chunk would have it

    TRACE_DMA_BEGIN(DDR_RD_DMA_CHANNEL_1, handle);
    // TRACE_EVENT(T_DDR_RD, 1, handle);
    DDR_read(srcdata, VSPA_HALF_WORDS(dest_block->addr), DDR_RD_DMA_CHANNEL_1, DAC_DMA_STEP);

    // pipe->ddr.input.bytes_done += DAC_DMA_STEP;

    if (ddr_src.flow.consumed == ddr_src.batch.size) {
        dma_table_pop(&DMA_TABLE);
        dest_block->flags |= PKT_IRQ | PKT_DMA_TCD_END;
        ddr_src_reset();
    }
    fifo_push(pipe->ddr.input.fifo, handle);
    TRACE_COUNTER(CNT_DDR_RD_ENQ, fifo_size(pipe->ddr.input.fifo));

    TRACE_DURATION(T_DDR_ENQ, DEFAULT_THREAD_ID, t1);
}

inline static void dac_completion(tx_pipeline_t *pipe) {
    TRACE_START_DURATION(t1);
    dmac_clear_complete(TX_DAC_DMA_MASK);
    dmac_clear_event(TX_DAC_DMA_MASK);

    const MetaHandle_t handle = fifo_front(pipe->dac.output.fifo);
    TRACE_DMA_END(TX_DAC_WR_DMA_CHANNEL, handle);
    // TRACE_EVENT(T_DAC, 1, handle);
    fifo_pop(pipe->dac.output.fifo);
    TRACE_COUNTER(CNT_DAC_ENQ, fifo_size(pipe->dac.output.fifo));

    pipe->dac.output.bytes_done += DAC_DMA_STEP;
    handles_stack_push(&handles_pool, handle);
    if (TX_CONTROL.generate_tone)
        TxGenerateTone(); // generate and enqueue next data batch

    TRACE_DURATION(T_AXIQ_COMPLETE, DEFAULT_THREAD_ID, t1);
}

inline static void dac_enqueue(tx_pipeline_t *pipe) {
    TRACE_START_DURATION(t1);
    // dmac_error_count(TX_DAC_DMA_MASK, &player_state.data_flow.tx_issues.xfer_errors);
    bool hadIssues = check_dac_had_issues();

    const MetaHandle_t handle = fifo_front(pipe->dac.input.fifo);
    const MemoryBlock_t *block = &meta_blocks[handle];
    uint32_t dma_flags = DMAC_FIFO | DMAC_WRC | TX_DAC_WR_DMA_CHANNEL | DMAC_TRIG_VCPU;

    const uint32_t tx_dma_allowed = gpird(1, BIT(16));
    // TRACE_COUNTER(CNT_TX_DMA_ALLOW, tx_dma_allowed);

    TRACE_DMA_BEGIN(TX_DAC_WR_DMA_CHANNEL, handle);

    // TRACE_COUNTER(CNT_PHYTIME, get_phytimer_counter());
    TRACE_EVENT(T_DAC, DEFAULT_THREAD_ID, handle);
    // DAC is 12bit, AXIQ does conversion 16bit -> 12bit, by rounding 4LSB and downshifting
    stream_write(dma_flags, dac_axi_fifo_addr, VSPA_HALF_WORDS(block->addr), DAC_DMA_STEP);
    pipe->dac.input.bytes_done += DAC_DMA_STEP;
    uint16_t timer_dma = 0;
    if (block->flags & PKT_HAS_TIMESTAMP) {
        if (block->flags & PKT_START) {
            if (tx_dma_allowed) {
                // schedule timer
                return; // tx_dma_allowed is enabled, cannot schedule next phytimer start, until current transmission window is
                        // terminated
            }
            timer_dma = timer_trig_schedule_async(TX_DMA_ALLOWED_TIMER, ePhyTimerComparatorOut1, block->timestamp);
            // if (!timer_dma) {
            //     ++player_state.data_flow.tx_issues.xfer_errors;
            //     TRACE_EVENT(T_ERROR, 1, 2);
            //     break;
            // }
        } else if (block->flags & PKT_END) {
            wait_tx_burst_end = true;
            TRACE_COUNTER(CNT_POOL, wait_tx_burst_end);
            tx_flush_stage = DAC_FLUSH_STAGE_ABORT;
            // dma_flags |= DMAC_FIFO_RESET; // reset AXIQ FIFO at the end of transfer
            // timer_dma = timer_trig_schedule_async(TX_DMA_ALLOWED_TIMER, ePhyTimerComparatorOut0, block->timestamp +
            // TX_DAC_DMA_SAMPLE_COUNT + 16);
            timer_dma = timer_trig_tx_end_async(block->timestamp + TX_DAC_DMA_SAMPLE_COUNT);
            if (!timer_dma) {
                ++player_state.data_flow.tx_issues.xfer_config_errors;
                TRACE_EVENT(T_ERROR, 1, 2);
            }
        }
    } else if (!tx_dma_allowed) {
        // no timestamps provided, start transmitting immediately
        timer_trig_immediate(TX_DMA_ALLOWED_TIMER, ePhyTimerComparatorOut1);
    }

    fifo_push(pipe->dac.output.fifo, handle);
    TRACE_COUNTER(CNT_DAC_ENQ, fifo_size(pipe->dac.output.fifo));

    fifo_pop(pipe->dac.input.fifo);
    TRACE_COUNTER(CNT_DAC_READY, fifo_size(pipe->dac.input.fifo));
    TRACE_DURATION(T_AXIQ_ENQ, 1, t1);
}

static void tone_enqueue(tx_pipeline_t *pipe) {
    for (int i = 0; i < MAX_DMA_ENQUE_COUNT; ++i) {
        if (fifo_size(pipe->ddr.output.fifo) > 1)
            return; // avoid consuming all memory pool

        if (handles_stack_isempty(&handles_pool))
            return;

        MetaHandle_t handle = handles_stack_top(&handles_pool);
        handles_stack_pop(&handles_pool);
        TRACE_COUNTER(CNT_POOL, handles_pool.count);

        gen_nco_single_tone(meta_blocks[handle].addr, TX_DAC_DMA_SAMPLE_COUNT / TX_CONFIG.oversample, &tx_tone_state);
        fifo_push(pipe->ddr.output.fifo, handle);
    }
}

void OnDDRRD_Completed(void) {
    TRACE_START_DURATION(t1);
    // if (!TX_CONTROL.ddr_enabled && TX_CONTROL.generate_tone)
    //     tone_enqueue(&txpipe);
    // else {
    ddr_completion(&txpipe);
    // }
    if (TX_CONFIG.oversample > 1)
        interpolate(&txpipe);

    if (should_dac_enqueue(&txpipe)) {
        dac_enqueue(&txpipe);

        // need two enques for the start of tx burst
        if (TX_CONFIG.oversample > 1)
            interpolate(&txpipe);
        if (should_ddr_enqueue(&txpipe))
            ddr_enqueue(&txpipe);
    }
    TRACE_DURATION(T_DDR_RD, DEFAULT_THREAD_ID, t1);
}

void OnDACWrite_Completed(void) {
    TRACE_START_DURATION(t1);
    dac_completion(&txpipe);

    if (dmac_is_complete(DDR_RD_DMA_MASK))
        ddr_completion(&txpipe);

    if (TX_CONFIG.oversample > 1)
        interpolate(&txpipe);

    if (should_dac_enqueue(&txpipe))
        dac_enqueue(&txpipe);

    if (dmac_is_complete(DDR_RD_DMA_MASK))
        ddr_completion(&txpipe);
    if (should_ddr_enqueue(&txpipe))
        ddr_enqueue(&txpipe);
    TRACE_DURATION(T_DAC, DEFAULT_THREAD_ID, t1);
}

void HostProducedEvent(void) {
    // TRACE_EVENT(T_HOST_PRODUCE, 1, 1);
    if (should_ddr_enqueue(&txpipe))
        ddr_enqueue(&txpipe);
}

lime_Result TxDMASubmit() {
    TRACE_EVENT(T_HOST_PRODUCE, DEFAULT_THREAD_ID, 1);
    if (dma_table_isfull(&DMA_TABLE))
        return lime_Result_Busy;

    dma_line_t line;
    line.timestamp = tx_dma_interface.timestamp;
    line.addr = tx_dma_interface.la9310_mem_address;
    line.size = tx_dma_interface.size;
    line.flags = tx_dma_interface.flags;
    dma_table_push(&DMA_TABLE, &line);

    if (should_ddr_enqueue(&txpipe)) {
        ddr_enqueue(&txpipe);
        // in case of new transmission start enque two buffers
        if (should_ddr_enqueue(&txpipe))
            ddr_enqueue(&txpipe);
    }
    return lime_Result_Success;
}

bool DAC_Flush(void) {
    TRACE_COUNTER(CNT_PHYTIME, tx_flush_stage);
    switch (tx_flush_stage) {
    default:
    case DAC_FLUSH_STAGE_NONE:
        return true;
    case DAC_FLUSH_STAGE_ABORT: {
        TxAXIQ(false); // enters DMA FLUSH mode
        tx_flush_stage = DAC_FLUSH_STAGE_WAIT_DMA_END;
        TRACE_COUNTER(CNT_PHYTIME, tx_flush_stage);
        if (dmac_is_enabled(TX_DAC_DMA_MASK)) {
            dmac_abort(TX_DAC_DMA_MASK);
            return false;
        }
        // dma was not active, fallthrough
    }
    case DAC_FLUSH_STAGE_WAIT_DMA_END: {
        tx_flush_stage = DAC_FLUSH_STAGE_FIFO_RST;
        TRACE_COUNTER(CNT_PHYTIME, tx_flush_stage);
        if (dmac_is_running(TX_DAC_DMA_MASK))
            return false; // still flushing
        // fall through to fifo rst
    }
    case DAC_FLUSH_STAGE_FIFO_RST: {
        if (dmac_is_running(TX_DAC_DMA_MASK))
            return false; // still flushing
        stream_write_ptr_rst(TX_DAC_WR_DMA_CHANNEL, dac_axi_fifo_addr); // must be done to exit DMA FLUSH mode
        tx_flush_stage = DAC_FLUSH_STAGE_DONE;
        TRACE_COUNTER(CNT_PHYTIME, tx_flush_stage);
        timer_trig_immediate_async(VSPA_GO_PHYTIMER_ID, ePhyTimerComparatorOut0); // GO triggered only by rising edge
        return false;
    }
    case DAC_FLUSH_STAGE_DONE: {
        if (!dmac_is_complete(TX_DAC_DMA_MASK))
            return false; // ptr rst not done yet

        wait_tx_burst_end = false;
        dmac_clear_complete(TX_DAC_DMA_MASK);
        TxAXIQ(true);
        // previous transmission has ended, start feeding data for next burst
        if (should_dac_enqueue(&txpipe))
            dac_enqueue(&txpipe);
        if (should_dac_enqueue(&txpipe))
            dac_enqueue(&txpipe);
        tx_flush_stage = DAC_FLUSH_STAGE_NONE;
        TRACE_COUNTER(CNT_PHYTIME, tx_flush_stage);
        return true;
    }
    }
    return false;
}