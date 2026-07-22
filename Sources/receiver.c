// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#include "dmac.h"
#include "dfe.h"
#include "receiver.h"
#include "l1-trace.h"
#include "vspa_state.h"
#include "dma_common.h"
#include "pipelines.h"
#include "memory_pool.h"
#include "fifo.h"
#include "tone_generator.h"

#include "platform.h"
#include "vcpu.h"

#include "compiler.h"
#include "ccnt.h"

#define MAX_DMA_ENQUE_COUNT 2
#define RX_ADC_BUF_COUNT 8 // (2 ADC, 2 DDR) per channel

#define DMA_XFER_SAMPLE_COUNT (512)
#define DMA_XFER_SIZE_BYTES (4 * DMA_XFER_SAMPLE_COUNT)

#define RX_CONFIG player_state.info.rx_config
#define RX_CONTROL player_state.internals.rx_control
#define RX_PIPELINE player_state.internals.rxpipe

static void ddr_enqueue(rx_pipeline_t *pipe);
static void decimation_buffer_reset(void);

D_STATIC tone_state_t rx_tone_state[RX_NUM_MAX_CHAN];
D_STATIC enum axiq_fifo_e Rx_Antenna2fifo_index[4];

static MemoryBlock_t meta_blocks[RX_ADC_BUF_COUNT];
D_STATIC cfixed16_t adc_buffer[RX_ADC_BUF_COUNT * DMA_XFER_SAMPLE_COUNT] __attribute__((aligned(64), section(".ippu_dmem")));

// Decimation filter state
D_STATIC cfixed16_t rx_decimation_history[RX_NUM_MAX_CHAN][32] __attribute__((aligned(64), section(".vcpu_dmem"))) = { 0 };

// Filter coefficients, must be gain normalized
const float rx_filter_taps_downsampling[8] __attribute__((aligned(64))) = {
#include "fir_decimation_x2_x4.txt"
};

static inline bool should_ddr_enqueue(rx_pipeline_t *pipe) {
    return dmac_is_available(1 << pipe->ddr_dma_channel) && !fifo_isempty(pipe->ddr.input.fifo) &&
           RX_CONTROL[pipe->channelIndex].ddr_enabled;
}

D_STATIC rx_pipeline_t *active_pipes[4];
D_STATIC uint16_t active_pipes_count = 0;

uint32_t RxChannelSelect(uint32_t channel_mask) {
    active_pipes_count = 0;
    for (int i = 0; i < 4; ++i) {
        if (channel_mask & (1 << i)) {
            active_pipes[active_pipes_count] = &RX_PIPELINE[i];
            // sequentially assign DDR_WR channels, top ones might be used for non Rx data purposes
            active_pipes[active_pipes_count]->ddr_dma_channel = DDR_WR_DMA_CHANNEL_1 + active_pipes_count;
            ++active_pipes_count;
            if (active_pipes_count > 2)
                return lime_Result_InvalidValue; // only allow two channels at the same time
            RxChannelConfigure((e_rx_channel)i, RX_CONFIG[i].oversample);
        }
    }
    player_state.info.rx_num_chan = active_pipes_count;
    return lime_Result_Success;
}

// #define RX_ADC_FIFO_BEAT_COUNT 16
// #define addr_beat_offset (0x1000 - RX_ADC_FIFO_BEAT_COUNT*16)
#define addr_beat_offset 0

void InitializeRx(void) {
    player_state.info.rx_num_chan = active_pipes_count;
    for (uint32_t i = 0; i < 4; ++i) {
        rx_config_t *cfg = &RX_CONFIG[i];

        cfg->oversample = 1;
        cfg->ddr_base_address = 0xdeadbeef;
        cfg->ddr_size = 0;
        cfg->ddr_step = DMA_XFER_SIZE_BYTES;

        rx_tone_state[i].amplitude = 0.9;
        rx_tone_state[i].phase = 0;
        rx_tone_state[i].freq_bin = 8192 / cfg->oversample;
    }

    RX_PIPELINE[VSPA_RO0].channelIndex = VSPA_RO0;
    RX_PIPELINE[VSPA_RO0].adc_dma_channel = RO0_ADC_RD_DMA_CHANNEL;
    RX_PIPELINE[VSPA_RO0].adc_axi_fifo_addr = 0x44001000 + addr_beat_offset;
    RX_PIPELINE[VSPA_RO0].ddr_dma_channel = DDR_WR_DMA_CHANNEL_3; // DDR_WR_DMA_CHANNEL_1;
    Rx_Antenna2fifo_index[VSPA_RO0] = AXIQ_FIFO_RX0;

    RX_PIPELINE[VSPA_RO1].channelIndex = VSPA_RO1;
    RX_PIPELINE[VSPA_RO1].adc_dma_channel = R01_ADC_RD_DMA_CHANNEL;
    RX_PIPELINE[VSPA_RO1].adc_axi_fifo_addr = 0x44002000 + addr_beat_offset;
    RX_PIPELINE[VSPA_RO1].ddr_dma_channel = DDR_WR_DMA_CHANNEL_1; // DDR_WR_DMA_CHANNEL_2;
    Rx_Antenna2fifo_index[VSPA_RO1] = AXIQ_FIFO_RX1;

    RX_PIPELINE[VSPA_RX0].channelIndex = VSPA_RX0;
    RX_PIPELINE[VSPA_RX0].adc_dma_channel = RX0_ADC_RD_DMA_CHANNEL;
    RX_PIPELINE[VSPA_RX0].adc_axi_fifo_addr = 0x44003000 + addr_beat_offset;
    RX_PIPELINE[VSPA_RX0].ddr_dma_channel = DDR_WR_DMA_CHANNEL_3;
    Rx_Antenna2fifo_index[VSPA_RX0] = AXIQ_FIFO_RX2;

    RX_PIPELINE[VSPA_RX1].channelIndex = VSPA_RX1;
    RX_PIPELINE[VSPA_RX1].adc_dma_channel = RX1_ADC_RD_DMA_CHANNEL;
    RX_PIPELINE[VSPA_RX1].adc_axi_fifo_addr = 0x44004000 + addr_beat_offset;
    RX_PIPELINE[VSPA_RX1].ddr_dma_channel = DDR_WR_DMA_CHANNEL_1;
    Rx_Antenna2fifo_index[VSPA_RX1] = AXIQ_FIFO_RX3;

    // by default activate RX0 channel
    RxChannelSelect(1 << VSPA_RX0);
    EnqueueProxyUpdate(PROXY_UPDATE_FLOW | PROXY_UPDATE_INFO | PROXY_UPDATE_INTERNALS);
}

static void InitMem(HandlesStack_t *pool, cfixed16_t *mem_ptr, uint32_t samples_in_block, uint32_t block_count) {
    handles_stack_clear(pool);
    for (int i = 0; i < block_count; ++i) {
        meta_blocks[i].addr = &mem_ptr[samples_in_block * (block_count - i - 1)];
        meta_blocks[i].size = DMA_XFER_SIZE_BYTES;
        meta_blocks[i].flags = 0;
        meta_blocks[i].timestamp = 0;
        handles_stack_push(pool, i);
    }
}

struct RxBuffering {
    struct MemoryFIFO adc_enq_fifo;
    struct MemoryFIFO processed_blocks_fifo;
    struct MemoryFIFO ddr_enq_fifo;
};

D_STATIC struct RxBuffering rx_buffering[RX_NUM_MAX_CHAN];

static void pipeline_setup(rx_pipeline_t *pipe) {
    struct RxBuffering *buffering = &rx_buffering[pipe->channelIndex];
    fifo_reset(&buffering->adc_enq_fifo);
    fifo_reset(&buffering->processed_blocks_fifo);
    fifo_reset(&buffering->ddr_enq_fifo);
    stage_setup(&pipe->adc, &buffering->adc_enq_fifo, &buffering->processed_blocks_fifo);
    stage_setup(&pipe->ddr, &buffering->processed_blocks_fifo, &buffering->ddr_enq_fifo);
    memclr(rx_decimation_history[pipe->channelIndex], sizeof(rx_decimation_history[0]));
}

static inline void stream_read_ptr_rst(uint32_t dma_channel, uint32_t axi_rd) {
    dmac_enable(DMAC_FIFO_RESET | DMAC_RDC | dma_channel, // ctrl flags
                DMA_XFER_SAMPLE_COUNT * VSPA_HALF_WORDS(sizeof(cfixed16_t)), // size
                axi_rd, // src
                VSPA_HALF_WORDS(adc_buffer) // dest
    );
}

static inline void stream_read(uint32_t dma_channel, uint32_t size, uint32_t axi_rd, uint32_t vsp) {
    // convert from two's complement out of the ADC/DAC to signed magnitude for local VSPA work
    dmac_enable(DMAC_RDC | DMAC_FIFO | dma_channel | DMAC_TRIG_VCPU, // flags
                size, // size
                axi_rd, // src
                vsp // dest
    );
}

static inline void DDR_write(uint32_t DDR_address, uint32_t vsp_address, uint16_t DDR_wr_dma_channel, uint16_t bytes_size) {
    dmac_enable(DMAC_WRC | DDR_wr_dma_channel, // | DMAC_TRIG_VCPU, // flags
                bytes_size, // size
                DDR_address, // src
                vsp_address // dest
    );
}

void ConfigRxHostFIFO(e_rx_channel index, uint32_t addr, uint32_t size) {
    RX_CONFIG[index].ddr_base_address = addr;
    RX_CONFIG[index].ddr_size = size;
}

void RxDMAFlush(e_rx_channel index) {
    const rx_pipeline_t *pipe = &RX_PIPELINE[index];
    // Disable Rx and reset fifo
    const enum axiq_fifo_e axi_fifo_index = Rx_Antenna2fifo_index[pipe->channelIndex];
    axiq_fifo_rx_enable(AXIQ_BANK_0, axi_fifo_index);
    axiq_fifo_rx_disable(AXIQ_BANK_0, axi_fifo_index);

    const uint32_t dma_mask = (1 << pipe->adc_dma_channel);
    dmac_abort(dma_mask);

    WAIT_TIMEOUT_R(!dmac_is_running(dma_mask) || dmac_errxfr(dma_mask), 10000);
    stream_read_ptr_rst(pipe->adc_dma_channel, pipe->adc_axi_fifo_addr);
    WAIT_TIMEOUT_R(dmac_is_complete(dma_mask) || dmac_errxfr(dma_mask), 10000);
    dmac_clear_complete(dma_mask);
    dmac_clear_event(dma_mask);
    dmac_clear_errxfr(dma_mask);
}

static void ResetRxSession(e_rx_channel index) {
    decimation_buffer_reset();

    RxDMAFlush(index);

    // uint32_t value = 0;
    // if (RX_ADC_FIFO_BEAT_COUNT == 16)
    //     value = 0x0;
    // else if (RX_ADC_FIFO_BEAT_COUNT == 8)
    //     value = 0x1;
    // else if (RX_ADC_FIFO_BEAT_COUNT == 4)
    //     value = 0x2;
    // else if (RX_ADC_FIFO_BEAT_COUNT == 2)
    //     value = 0x3;
    // const uint32_t mask = (0x3 << 1) | (0x3 << 9) | (0x3 << 17) | (0x3 << 25);
    // gpowr(4, mask, (value << 1) | (value << 9) | (value << 17) | (value << 25));

    const enum axiq_fifo_e axi_fifo_index = Rx_Antenna2fifo_index[RX_PIPELINE[index].channelIndex];
    axiq_fifo_rx_enable(AXIQ_BANK_0, axi_fifo_index);
    axiq_fifo_rx_cr(AXIQ_BANK_0, axi_fifo_index, AXIQ_CR_CLRERR, AXIQ_CR_CLRERR);
    axiq_fifo_rx_cr(AXIQ_BANK_0, axi_fifo_index, AXIQ_CR_CLRERR, 0);
}

lime_Result RxChannelConfigure(e_rx_channel index, uint32_t decimation) {
    if (index > 3)
        return lime_Result_InvalidValue;
    rx_config_t *cfg = &RX_CONFIG[index];
    rx_control_t *ctrl = &RX_CONTROL[index];
    rx_pipeline_t *pipe = &RX_PIPELINE[index];

    cfg->oversample = decimation;
    cfg->ddr_step = DMA_XFER_SIZE_BYTES;

    ctrl->generate_tone = 0;
    ctrl->host_flow_control_disable = 0; //(HIWORD(msg64)) & 0x00400000;
    player_state.data_flow.rx[index].produced = 0;

    EnqueueProxyUpdate(PROXY_UPDATE_INFO | PROXY_UPDATE_FLOW);
    return lime_Result_Success;
}

lime_Result RxPrepare() {
    cfixed16_t *buf = adc_buffer;
    for (uint32_t c = 0; c < active_pipes_count; ++c) {
        rx_pipeline_t *pipe = &RX_PIPELINE[active_pipes[c]->channelIndex];
        InitMem(&pipe->mem_handles_pool, buf, DMA_XFER_SAMPLE_COUNT, RX_ADC_BUF_COUNT / active_pipes_count);
        buf += DMA_XFER_SAMPLE_COUNT * (RX_ADC_BUF_COUNT / active_pipes_count);
        pipeline_setup(pipe);
        RxDMAFlush(pipe->channelIndex);
    }
    EnqueueProxyUpdate(PROXY_UPDATE_INFO | PROXY_UPDATE_FLOW);
    return lime_Result_Success;
}

static inline void adc_enqueue(rx_pipeline_t *pipe) {
    TRACE_START_DURATION(t1);

    if (handles_stack_isempty(&pipe->mem_handles_pool)) {
        TRACE_EVENT(T_NO_MEMORY, 1, pipe->adc_dma_channel);
        TRACE_DURATION(T_AXIQ_ENQ, 1, t1);
        return;
    }

    const MetaHandle_t handle = handles_stack_top(&pipe->mem_handles_pool);
    handles_stack_pop(&pipe->mem_handles_pool);

    TRACE_DMA_BEGIN(pipe->adc_dma_channel, handle);
    TRACE_EVENT(T_ADC_ENQ, DEFAULT_THREAD_ID, handle);

    stream_read(pipe->adc_dma_channel, DMA_XFER_SIZE_BYTES, pipe->adc_axi_fifo_addr, VSPA_HALF_WORDS(meta_blocks[handle].addr));

    fifo_push(pipe->adc.input.fifo, handle);

    pipe->adc.input.bytes_done += DMA_XFER_SIZE_BYTES;
    TRACE_COUNTER(CNT_ADC_ENQ, fifo_size(pipe->adc.input.fifo));
    TRACE_DURATION(T_AXIQ_ENQ, 1, t1);
}

lime_Result RxDDR_control(e_rx_channel index, uint64_t msg64) {
    if (index > 3)
        return lime_Result_InvalidValue;

    rx_control_t *ctrl = &RX_CONTROL[index];
    const bool ddr_enable = (HIWORD(msg64)) & (1 << 0);
    const bool reset_pipeline = (HIWORD(msg64)) & (1 << 1);

    if (reset_pipeline) {
        if (ctrl->ddr_enabled || ctrl->generate_tone)
            return lime_Result_Busy;
        RxChannelConfigure(index, RX_CONFIG[index].oversample);
    }

    if (ctrl->ddr_enabled == ddr_enable)
        return lime_Result_Success;

    const rx_pipeline_t *pipe = &RX_PIPELINE[index];
    if (ddr_enable) {
        ctrl->ddr_enabled = true;
        ResetRxSession(pipe->channelIndex);

        memclr(&player_state.data_flow.rx[pipe->channelIndex], sizeof(player_state.data_flow.rx[pipe->channelIndex]));
        memclr(&player_state.data_flow.rx_issues[pipe->channelIndex], sizeof(player_state.data_flow.rx_issues[pipe->channelIndex]));

        const uint16_t dma_mask = 0x1 << pipe->adc_dma_channel;
        dmac_clear_complete(dma_mask);
        dmac_clear_errxfr(dma_mask);
        EnqueueProxyUpdate(PROXY_UPDATE_FLOW);

        for (uint8_t c = 0; c < active_pipes_count; ++c) {
            for (uint16_t i = 0; i < MAX_DMA_ENQUE_COUNT; ++i)
                adc_enqueue(active_pipes[c]);
        }
    } else {
        ctrl->ddr_enabled = false;
        axiq_fifo_rx_disable(AXIQ_BANK_0, Rx_Antenna2fifo_index[pipe->channelIndex]);
        EnqueueProxyUpdate(PROXY_UPDATE_FLOW | PROXY_UPDATE_INFO | PROXY_UPDATE_INTERRUPT);
    }
    return lime_Result_Success;
}

static inline void check_adc_axi_status(const rx_pipeline_t *pipe) {
    const uint8_t ci = pipe->channelIndex;
    const enum axiq_fifo_e fifo_index = Rx_Antenna2fifo_index[ci];
    const uint8_t field_shift = axiq_sr_shift(fifo_index);
    // Check AXIQ rx fifo is not full or overrun
    uint32_t status = axiq_fifo_rx_sr(AXIQ_BANK_0, fifo_index, AXIQ_SR_FIELD_ERROVER | AXIQ_SR_FIELD_ERRUNDER);
    if (status == 0)
        return;

    status >>= field_shift;
    if (status & AXIQ_SR_FIELD_ERROVER) {
        ++player_state.data_flow.rx_issues[ci].overrun;
        TRACE_COUNTER(CNT_RX0_OVR + pipe->channelIndex, player_state.data_flow.rx_issues[ci].overrun);
    }
    if (status & AXIQ_SR_FIELD_ERRUNDER) {
        ++player_state.data_flow.rx_issues[ci].underrun;
        TRACE_COUNTER(CNT_RX0_UDR + pipe->channelIndex, player_state.data_flow.rx_issues[ci].underrun);
    }
    EnqueueProxyUpdate(PROXY_UPDATE_FLOW);
    axiq_fifo_rx_cr(AXIQ_BANK_0, fifo_index, AXIQ_CR_CLRERR, AXIQ_CR_CLRERR);
    axiq_fifo_rx_cr(AXIQ_BANK_0, fifo_index, AXIQ_CR_CLRERR, 0);
}

// ddc2x4x.sx prototypes
extern void decimator_2x_8_Taps_asm(cfixed16_t *pOut, cfixed16_t *pIn, float32_t *pTaps, cfixed16_t *filtState, uint32_t n_samples);
extern void decimator_4x_8_Taps_asm(cfixed16_t *pOut, cfixed16_t *pIn, float32_t *pTaps, cfixed16_t *filtState, uint32_t n_samples);

static MetaHandle_t decimated_block_handle = INVALID_HANDLE;
static uint16_t decimated_samples_accumulated = 0;

static inline void decimation_buffer_reset(void) {
    decimated_block_handle = INVALID_HANDLE;
    decimated_samples_accumulated = 0;
}

static inline void decimate(rx_pipeline_t *pipe, struct MemoryFIFO *out_fifo, MetaHandle_t handle) {
    TRACE_START_DURATION(t1);

    if (decimated_block_handle == INVALID_HANDLE) {
        // keep the first buffer for intermediate storage
        decimated_block_handle = handle;
        decimated_samples_accumulated = 0;
    } else {
        // subsequent buffers returned to pool.
        // pushing here, to avoid conditional check again after work.
        // the handle remains valid until popped elsewhere.
        handles_stack_push(&pipe->mem_handles_pool, handle);
    }

    cfixed16_t *out = (cfixed16_t *)meta_blocks[decimated_block_handle].addr + decimated_samples_accumulated;
    switch (RX_CONFIG[pipe->channelIndex].oversample) {
    default:
    case 2:
        decimator_2x_8_Taps_asm(out, meta_blocks[handle].addr, (float *)rx_filter_taps_downsampling,
                                rx_decimation_history[pipe->channelIndex], DMA_XFER_SAMPLE_COUNT);
        break;
    case 4:
        decimator_4x_8_Taps_asm(out, meta_blocks[handle].addr, (float *)rx_filter_taps_downsampling,
                                rx_decimation_history[pipe->channelIndex], DMA_XFER_SAMPLE_COUNT);
        break;
    }

    decimated_samples_accumulated += DMA_XFER_SAMPLE_COUNT / RX_CONFIG[pipe->channelIndex].oversample;
    if (decimated_samples_accumulated == DMA_XFER_SAMPLE_COUNT) {
        fifo_push(out_fifo, decimated_block_handle);
        TRACE_COUNTER(CNT_ADC_READY, fifo_size(out_fifo));
        decimated_block_handle = INVALID_HANDLE;
        if (should_ddr_enqueue(pipe))
            ddr_enqueue(pipe);
    }

    TRACE_DURATION(T_DEC_BUFFER, 1, t1);
}

static inline void adc_completion(rx_pipeline_t *pipe) {
    TRACE_START_DURATION(t1);

    const uint32_t dma_mask = 0x1 << pipe->adc_dma_channel;

    // if (dmac_errxfr(dma_mask))
    // {
    //     dmac_clear_errxfr(dma_mask);
    //     player_state.data_flow.rx_issues[pipe->channelIndex].xfer_errors++;
    //     TRACE_EVENT(TG_DMA, T_XFER_ERROR, player_state.data_flow.rx_issues[pipe->channelIndex].xfer_errors,
    //     pipe->adc_dma_channel);
    // }

    dmac_clear_complete(dma_mask);
    dmac_clear_event(dma_mask);

    const uint16_t handle = fifo_front(pipe->adc.input.fifo);
    fifo_pop(pipe->adc.input.fifo);
    TRACE_COUNTER(CNT_ADC_ENQ, fifo_size(pipe->adc.input.fifo));
    TRACE_DMA_END(pipe->adc_dma_channel, handle);

    // if (RX_CONTROL[pipe->channelIndex].generate_tone) // replace adc data with generated signal
    //     gen_nco_single_tone(chunk->addr, DMA_XFER_SAMPLE_COUNT, &rx_tone_state[pipe->channelIndex]);
    {
        TRACE_START_DURATION(t2);
        // in place processing
        rx_qec_correction(meta_blocks[handle].addr, meta_blocks[handle].addr, DMA_XFER_SAMPLE_COUNT);
        TRACE_DURATION(T_QEC_RX_BUFFER, DEFAULT_THREAD_ID, t2);
    }

    if (RX_CONFIG[pipe->channelIndex].oversample > 1)
        decimate(pipe, pipe->adc.output.fifo, handle);
    else
        fifo_push(pipe->adc.output.fifo, handle);

    pipe->adc.output.bytes_done += DMA_XFER_SIZE_BYTES;

    TRACE_COUNTER(CNT_ADC_READY, fifo_size(pipe->adc.output.fifo));
    TRACE_DURATION(T_ADC_COMPLETE, DEFAULT_THREAD_ID, t1);
}

static inline void ddr_completion(rx_pipeline_t *pipe) {
    TRACE_START_DURATION(t1);
    const uint32_t dma_mask = (1 << pipe->ddr_dma_channel);

    // this is called by DMA GO event, so completion is assumed to be true
    // if (dmac_is_complete(dma_mask) != dma_mask)
    //     return;

    dmac_clear_complete((1 << pipe->ddr_dma_channel));
    dmac_clear_event((1 << pipe->ddr_dma_channel));

    const MetaHandle_t handle = fifo_front(pipe->ddr.output.fifo);

    TRACE_DMA_END(pipe->ddr_dma_channel, handle);

    pipe->ddr.output.bytes_done += DMA_XFER_SIZE_BYTES;
    player_state.data_flow.rx[pipe->channelIndex].produced = pipe->ddr.output.bytes_done;

    handles_stack_push(&pipe->mem_handles_pool, handle);

    fifo_pop(pipe->ddr.output.fifo);
    TRACE_COUNTER(CNT_DDR_WR_ENQ, fifo_size(pipe->ddr.output.fifo));
    if ((pipe->ddr.output.bytes_done & (0x1FFFFu)) == 0) {
        // interupt every 64KB
        EnqueueProxyUpdate(PROXY_UPDATE_FLOW | PROXY_UPDATE_INTERRUPT);
    }
    TRACE_DURATION(T_DDR_WR_COMPLETE, DEFAULT_THREAD_ID, t1);
}

static inline void ddr_enqueue(rx_pipeline_t *pipe) {
    TRACE_START_DURATION(t1);

    const MetaHandle_t handle = fifo_front(pipe->ddr.input.fifo);

    const uint32_t destaddr =
        RX_CONFIG[pipe->channelIndex].ddr_base_address + (pipe->ddr.input.bytes_done % (RX_CONFIG[pipe->channelIndex].ddr_size));
    TRACE_DMA_BEGIN(pipe->ddr_dma_channel, handle);
    // TRACE_EVENT(T_DDR_WR, 1, handle);
    DDR_write(destaddr, VSPA_HALF_WORDS(meta_blocks[handle].addr), pipe->ddr_dma_channel, DMA_XFER_SIZE_BYTES);

    pipe->ddr.input.bytes_done += DMA_XFER_SIZE_BYTES;
    fifo_pop(pipe->ddr.input.fifo);
    TRACE_COUNTER(CNT_ADC_READY, fifo_size(pipe->adc.output.fifo));

    fifo_push(pipe->ddr.output.fifo, handle);
    TRACE_COUNTER(CNT_DDR_WR_ENQ, fifo_size(pipe->ddr.output.fifo));

    TRACE_DURATION(T_DDR_WR, DEFAULT_THREAD_ID, t1);
}

void OnADCRead_Completed(e_rx_channel c) {
    TRACE_START_DURATION(t1);
    rx_pipeline_t *pipe = &RX_PIPELINE[c];
    check_adc_axi_status(pipe);

    adc_completion(pipe);

    if (dmac_is_complete(1 << pipe->ddr_dma_channel))
        ddr_completion(pipe);

    adc_enqueue(pipe); // try to queue as soon as possible, if ddr_completion freed up a buffer

    if (should_ddr_enqueue(pipe))
        ddr_enqueue(pipe);

    TRACE_DURATION(T_ADC_COMPLETE, DEFAULT_THREAD_ID, t1);
}

void OnDDRWR_Completed(uint16_t c) {
    TRACE_START_DURATION(t1);
    ddr_completion(active_pipes[c]);
    TRACE_DURATION(T_DDR_WR, DEFAULT_THREAD_ID, t1);
}
