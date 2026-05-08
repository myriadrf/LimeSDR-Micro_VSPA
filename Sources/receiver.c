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

extern void ProcessTx(void);

#define RX_ADC_DMA_SAMPLE_COUNT (512)
#define RX_ADC_BUF_COUNT 4

#define RX_DMA_TXR_STEP (4 * RX_ADC_DMA_SAMPLE_COUNT)

#define RX_CONFIG player_state.info.rx_config
#define RX_CONTROL player_state.internals.rx_control
#define RX_PIPELINE player_state.internals.rxpipe

#if 0
#define TRACE_RX(x) x
#else
#define TRACE_RX(x)
#endif

static tone_state_t rx_tone_state[RX_NUM_MAX_CHAN];
static enum axiq_fifo_e Rx_Antenna2fifo_index[4];

static cfixed16_t adc_buffer[RX_ADC_BUF_COUNT * RX_ADC_DMA_SAMPLE_COUNT] __attribute__((aligned(64), section(".ippu_dmem")));

// Decimation filter state
static cfixed16_t rx_decimation_history[RX_NUM_MAX_CHAN][32] __attribute__((aligned(64), section(".ippu_dmem"))) = { 0 };

// Filter coefficients, must be gain normalized
const float rx_filter_taps_downsampling[8] __attribute__((aligned(64))) = {
#include "fir_decimation_x2_x4.txt"
};

static MemoryPool_t vcpu_pool[RX_NUM_MAX_CHAN];

static rx_pipeline_t *active_pipes[4];
static uint32_t active_pipes_count = 0;

void RxChannelSelect(uint32_t channel_mask) {
    l1_trace(L1_TRACE_MSG_RX_CHANNEL_SELECT, channel_mask);
    active_pipes_count = 0;
    for (int i = 0; i < 4; ++i) {
        if (channel_mask & (1 << i)) {
            active_pipes[active_pipes_count] = &RX_PIPELINE[i];
            ++active_pipes_count;
            RxChannelConfigure((e_rx_channel)i, RX_CONFIG[i].oversample);
        }
    }
    player_state.info.rx_num_chan = active_pipes_count;
}

void InitializeRx(void) {
    player_state.info.rx_num_chan = active_pipes_count;
    for (uint32_t i = 0; i < 4; ++i) {
        rx_config_t *cfg = &RX_CONFIG[i];

        cfg->oversample = 1;
        cfg->ddr_base_address = 0xdeadbeef;
        cfg->ddr_size = 0;
        cfg->ddr_step = RX_DMA_TXR_STEP / cfg->oversample;

        rx_tone_state[i].amplitude = 0.9;
        rx_tone_state[i].phase = 0;
        rx_tone_state[i].freq_bin = 8192 / cfg->oversample;

        // memclr(&player_state.data_flow.rx[i], sizeof(player_state.data_flow.rx[i]));
    }

    RX_PIPELINE[VSPA_RO0].channelIndex = VSPA_RO0;
    RX_PIPELINE[VSPA_RO0].adc_dma_channel = 0x1;
    RX_PIPELINE[VSPA_RO0].adc_axi_fifo_addr = 0x44001000;
    RX_PIPELINE[VSPA_RO0].ddr_dma_channel = DDR_WR_DMA_CHANNEL_1;
    Rx_Antenna2fifo_index[VSPA_RO0] = AXIQ_FIFO_RX0;

    RX_PIPELINE[VSPA_RO1].channelIndex = VSPA_RO1;
    RX_PIPELINE[VSPA_RO1].adc_dma_channel = 0x2;
    RX_PIPELINE[VSPA_RO1].adc_axi_fifo_addr = 0x44002000;
    RX_PIPELINE[VSPA_RO1].ddr_dma_channel = DDR_WR_DMA_CHANNEL_2;
    Rx_Antenna2fifo_index[VSPA_RO1] = AXIQ_FIFO_RX1;

    RX_PIPELINE[VSPA_RX0].channelIndex = VSPA_RX0;
    RX_PIPELINE[VSPA_RX0].adc_dma_channel = 0x3;
    RX_PIPELINE[VSPA_RX0].adc_axi_fifo_addr = 0x44003000;
    RX_PIPELINE[VSPA_RX0].ddr_dma_channel = DDR_WR_DMA_CHANNEL_3;
    Rx_Antenna2fifo_index[VSPA_RX0] = AXIQ_FIFO_RX2;

    RX_PIPELINE[VSPA_RX1].channelIndex = VSPA_RX1;
    RX_PIPELINE[VSPA_RX1].adc_dma_channel = 0x4;
    RX_PIPELINE[VSPA_RX1].adc_axi_fifo_addr = 0x44004000;
    RX_PIPELINE[VSPA_RX1].ddr_dma_channel = DDR_WR_DMA_CHANNEL_4;
    Rx_Antenna2fifo_index[VSPA_RX1] = AXIQ_FIFO_RX3;

    // by default activate RX0 channel
    RxChannelSelect(1 << VSPA_RX0);
    RxChannelConfigure(VSPA_RX0, 1);
    EnqueueProxyUpdate(PROXY_UPDATE_FLOW | PROXY_UPDATE_INFO | PROXY_UPDATE_INTERNALS);
}

static void InitMem(MemoryPool_t *pool, cfixed16_t *mem_ptr, uint32_t samples_in_block, uint32_t block_count) {
    MemoryBlock_t block;
    mempool_clear(pool);
    for (int i = 0; i < block_count; ++i) {
        block.addr = &mem_ptr[samples_in_block * (block_count - i - 1)];
        block.size = RX_DMA_TXR_STEP;
        mempool_push(pool, &block);
    }
}

struct RxBuffering {
    struct MemoryFIFO adc_enq_fifo;
    struct MemoryFIFO adc_ready_fifo;
    struct MemoryFIFO qec_out_fifo;
    struct MemoryFIFO dec_out_fifo;
    struct MemoryFIFO ddr_enq_fifo;
};

static struct RxBuffering rx_buffering[RX_NUM_MAX_CHAN];

static void pipeline_setup(rx_pipeline_t *pipe) {
    struct MemoryFIFO *last_stage_fifo = NULL;
    struct RxBuffering *buffering = &rx_buffering[pipe->channelIndex];

    fifo_reset(&buffering->adc_enq_fifo);
    fifo_reset(&buffering->adc_ready_fifo);
    stage_setup(&pipe->adc, &buffering->adc_enq_fifo, &buffering->adc_ready_fifo);
    last_stage_fifo = &buffering->adc_ready_fifo;

    fifo_reset(&buffering->qec_out_fifo);
    stage_setup(&pipe->qec, last_stage_fifo, &buffering->qec_out_fifo);
    last_stage_fifo = &buffering->qec_out_fifo;

    if (RX_CONFIG[pipe->channelIndex].oversample > 1) {
        memclr(rx_decimation_history[pipe->channelIndex], sizeof(rx_decimation_history[0]));
        fifo_reset(&buffering->dec_out_fifo);
        stage_setup(&pipe->dec, last_stage_fifo, &buffering->dec_out_fifo);
        last_stage_fifo = &buffering->dec_out_fifo;
    }

    fifo_reset(&buffering->ddr_enq_fifo);
    stage_setup(&pipe->ddr, last_stage_fifo, &buffering->ddr_enq_fifo);
}

static void stream_read_ptr_rst(uint32_t dma_channel_mask, uint32_t axi_rd) {
    const uint32_t ctrl = DMAC_FIFO_RESET | DMAC_RD | DMAC_WRC | dma_channel_mask;
    const uint32_t vsp = VSPA_HALF_WORDS(adc_buffer);
    TRACE_RX(l1_trace_nr(L1_TRACE_MSG_DMA_PTR_RST, axi_rd));
    dmac_enable(ctrl, RX_ADC_DMA_SAMPLE_COUNT * VSPA_HALF_WORDS(sizeof(cfixed16_t)), axi_rd, vsp);
}

static inline void stream_read(uint32_t dma_channel_mask, uint32_t size, uint32_t axi_rd, uint32_t vsp) {
    // convert from two's complement out of the ADC/DAC to signed magnitude for local VSPA work
    const uint32_t ctrl = DMAC_RDC | DMAC_FIFO | dma_channel_mask;
    // l1_trace(L1_TRACE_MSG_DMA_AXIQ_RX,(uint32_t)(dma_channel_mask<<24) +(uint32_t)vsp);
    dmac_enable(ctrl, size, axi_rd, vsp);
    // l1_trace_dma(dma_channel_mask, L1_TRACE_MSG_DMA_AXIQ_RX_START, vsp);
}

static inline void DDR_write_multi_dma(uint32_t DDR_wr_dma_channel, uint8_t nb_dma, uint32_t DDR_address, uint32_t vsp_address,
                                       uint32_t bytes_size) {
    uint32_t size = bytes_size / nb_dma;
    // user should ensure bytes_size/nb_dma is enire and aligned on AXI bus width 16B.
    // 2048B is ok with 1,2,4 dmas only
    // dma channel should be contiguous

    for (uint8_t i = 0; i < nb_dma; i++) {
#pragma loop_count(1, 16, 2, 0)
        const uint32_t ctrl = DMAC_WRC | (DDR_wr_dma_channel + i);
        dmac_enable(ctrl, size, DDR_address + i * size, vsp_address + i * size);
    }
}

void RxHostFIFO(e_rx_channel index, uint32_t addr, uint32_t size) {
    RX_CONFIG[index].ddr_base_address = addr;
    RX_CONFIG[index].ddr_size = size;
    l1_trace(L1_TRACE_MSG_RX_FIFO_SET, addr);
}

void RxDMAFlush(e_rx_channel index) {
    const rx_pipeline_t *pipe = &RX_PIPELINE[index];
    // Disable Rx and reset fifo
    const enum axiq_fifo_e axi_fifo_index = Rx_Antenna2fifo_index[pipe->channelIndex];
    axiq_fifo_rx_enable(AXIQ_BANK_0, axi_fifo_index);
    TRACE_RX(l1_trace_nr(L1_TRACE_MSG_RX_AXIQ, 1));
    axiq_fifo_rx_disable(AXIQ_BANK_0, axi_fifo_index);
    TRACE_RX(l1_trace_nr(L1_TRACE_MSG_RX_AXIQ, 0));

    const uint32_t dma_mask = (1 << pipe->adc_dma_channel);
    dmac_abort(dma_mask);
    do {
        ProcessTx();
    } while (dmac_is_enabled(dma_mask) && !dmac_errxfr(dma_mask));
    stream_read_ptr_rst(pipe->adc_dma_channel, pipe->adc_axi_fifo_addr);
    dmac_clear_complete(dma_mask);
}

static void ResetRxSession(e_rx_channel index) {
    const rx_pipeline_t *pipe = &RX_PIPELINE[index];

    const enum axiq_fifo_e axi_fifo_index = Rx_Antenna2fifo_index[pipe->channelIndex];
    axiq_fifo_rx_disable(AXIQ_BANK_0, axi_fifo_index);

    const uint32_t dma_mask = (1 << pipe->adc_dma_channel);
    do {
        ProcessTx();
    } while (dmac_is_enabled(dma_mask) && !dmac_errxfr(dma_mask));
    stream_read_ptr_rst(pipe->adc_dma_channel, pipe->adc_axi_fifo_addr);
    do {
        ProcessTx();
    } while (dmac_is_enabled(dma_mask) && !dmac_errxfr(dma_mask));
    dmac_clear_complete(dma_mask);

    axiq_fifo_rx_enable(AXIQ_BANK_0, axi_fifo_index);
    axiq_fifo_rx_cr(AXIQ_BANK_0, axi_fifo_index, AXIQ_CR_CLRERR, AXIQ_CR_CLRERR);
    axiq_fifo_rx_cr(AXIQ_BANK_0, axi_fifo_index, AXIQ_CR_CLRERR, 0);
}

lime_Result RxChannelConfigure(e_rx_channel index, uint32_t decimation) {
    if (index > 3)
        return lime_Result_InvalidValue;
    l1_trace(L1_TRACE_MSG_RX_CONFIG, (index << 20) | decimation);
    rx_config_t *cfg = &RX_CONFIG[index];
    rx_control_t *ctrl = &RX_CONTROL[index];
    rx_pipeline_t *pipe = &RX_PIPELINE[index];

    ctrl->ddr_wr_dma_ch_nb = 1;
    ctrl->ddr_wr_dma_ch_mask = dma_chan_mask(pipe->ddr_dma_channel, ctrl->ddr_wr_dma_ch_nb);
    cfg->oversample = decimation;
    cfg->ddr_step = RX_DMA_TXR_STEP / cfg->oversample;

    ctrl->generate_tone = 0;
    ctrl->host_flow_control_disable = 0; //(HIWORD(msg64)) & 0x00400000;
    player_state.data_flow.rx[index].produced = 0;

    EnqueueProxyUpdate(PROXY_UPDATE_INFO | PROXY_UPDATE_FLOW | PROXY_UPDATE_INTERNALS);
    return lime_Result_Success;
}

lime_Result RxPrepare() {
    cfixed16_t *buf = adc_buffer;
    for (uint32_t c = 0; c < active_pipes_count; ++c) {
        rx_pipeline_t *pipe = &RX_PIPELINE[active_pipes[c]->channelIndex];
        InitMem(&vcpu_pool[pipe->channelIndex], buf, RX_ADC_DMA_SAMPLE_COUNT, RX_ADC_BUF_COUNT / active_pipes_count);
        buf += RX_ADC_DMA_SAMPLE_COUNT * (RX_ADC_BUF_COUNT / active_pipes_count);
        pipeline_setup(pipe);
        RxDMAFlush(pipe->channelIndex);
    }
    EnqueueProxyUpdate(PROXY_UPDATE_INFO | PROXY_UPDATE_FLOW | PROXY_UPDATE_INTERNALS);
    return lime_Result_Success;
}

lime_Result RxDDR_control(e_rx_channel index, uint64_t msg64) {
    if (index > 3)
        return lime_Result_InvalidValue;

    rx_control_t *ctrl = &RX_CONTROL[index];
    const bool ddr_enable = (HIWORD(msg64)) & (1 << 0);
    const bool reset_pipeline = (HIWORD(msg64)) & (1 << 1);
    l1_trace(L1_TRACE_MSG_RX_CONTROL, HIWORD(msg64));

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
        const enum axiq_fifo_e axi_fifo_index = Rx_Antenna2fifo_index[pipe->channelIndex];

        ResetRxSession(pipe->channelIndex);

        const uint16_t dma_mask = 0x1 << pipe->adc_dma_channel;
        dmac_clear_complete(dma_mask);
        dmac_clear_errxfr(dma_mask);
        EnqueueProxyUpdate(PROXY_UPDATE_FLOW);
    } else {
        ctrl->ddr_enabled = false;
        const enum axiq_fifo_e axi_fifo_index = Rx_Antenna2fifo_index[pipe->channelIndex];
        axiq_fifo_rx_disable(AXIQ_BANK_0, axi_fifo_index);

        EnqueueProxyUpdate(PROXY_UPDATE_FLOW | PROXY_UPDATE_INFO | PROXY_UPDATE_INTERRUPT);
    }
    return lime_Result_Success;
}

static inline void check_adc_axi_status(const rx_pipeline_t *pipe) {
    const uint8_t ci = pipe->channelIndex;
    const enum axiq_fifo_e fifo_index = Rx_Antenna2fifo_index[ci];
    const uint8_t field_shift = axiq_sr_shift(fifo_index);
    // Check AXIQ rx fifo is not full or overrun
    const uint32_t status = axiq_fifo_rx_sr(AXIQ_BANK_0, fifo_index, AXIQ_SR_FIELD_ERROVER | AXIQ_SR_FIELD_ERRUNDER);
    if (status != 0) {
        if (status & (AXIQ_SR_FIELD_ERROVER << field_shift)) {
            ++player_state.data_flow.rx_issues[ci].overrun;
            TRACE_RX(l1_trace_nr(L1_TRACE_MSG_DMA_AXIQ_RX_OVER, player_state.data_flow.rx_issues[ci].overrun));
        }
        if (status & (AXIQ_SR_FIELD_ERRUNDER << field_shift)) {
            ++player_state.data_flow.rx_issues[ci].underrun;
            TRACE_RX(l1_trace_nr(L1_TRACE_MSG_DMA_AXIQ_RX_UNDER, player_state.data_flow.rx_issues[ci].underrun));
        }
        // l1_trace_disable = 1;
        axiq_fifo_rx_cr(AXIQ_BANK_0, fifo_index, AXIQ_CR_CLRERR, AXIQ_CR_CLRERR);
        axiq_fifo_rx_cr(AXIQ_BANK_0, fifo_index, AXIQ_CR_CLRERR, 0);
    }
}

static inline void adc_completion(rx_pipeline_t *pipe) {
    const uint32_t dma_mask = 0x1 << pipe->adc_dma_channel;
    const uint32_t xfers_done = xfers_to_process(dma_mask, pipe->adc.input.fifo);
    if (!xfers_done)
        return;

    for (uint32_t i = 0; i < xfers_done; ++i) {
        MemoryBlock_t chunk;
        if (!fifo_pop(pipe->adc.input.fifo, &chunk))
            return;

        pipe->adc.output.bytes_done += chunk.size;
        TRACE_RX(l1_trace(L1_TRACE_MSG_DMA_AXIQ_RX_COMP, (uint32_t)chunk.addr));

        if (RX_CONTROL[pipe->channelIndex].generate_tone) // replace adc data with generated signal
            gen_nco_single_tone(chunk.addr, RX_ADC_DMA_SAMPLE_COUNT, &rx_tone_state[pipe->channelIndex]);

        fifo_push(pipe->adc.output.fifo, &chunk);
    }

    if (!dmac_errxfr(dma_mask))
        return;

    dmac_clear_errxfr(dma_mask);
    player_state.data_flow.rx_issues[pipe->channelIndex].xfer_errors++;
    TRACE_RX(l1_trace(L1_TRACE_MSG_DMA_AXIQ_RX_XFER_ERROR, player_state.data_flow.rx_issues[pipe->channelIndex].xfer_errors));
}

static inline void adc_enqueue(rx_pipeline_t *pipe) {
    if (!dmac_is_available(0x1 << pipe->adc_dma_channel))
        return;

    MemoryBlock_t chunk;
    if (!mempool_pop(&vcpu_pool[pipe->channelIndex], &chunk))
        return;

    const uint32_t adc_xfer_size = chunk.size;
    const uint32_t axi_rd = pipe->adc_axi_fifo_addr;
    stream_read(pipe->adc_dma_channel, adc_xfer_size, axi_rd, VSPA_HALF_WORDS(chunk.addr));
    pipe->adc.input.bytes_done += adc_xfer_size;

    TRACE_RX(l1_trace(L1_TRACE_MSG_DMA_AXIQ_RX_START, (uint32_t)chunk.addr));
    fifo_push(pipe->adc.input.fifo, &chunk);
}

static void qec_work(rx_pipeline_t *pipe) {
    if (fifo_isfull(pipe->qec.output.fifo))
        return;

    MemoryBlock_t chunk;
    if (!fifo_pop(pipe->qec.input.fifo, &chunk))
        return;

    // in place
    cfixed16_t *in = chunk.addr;
    cfixed16_t *out = chunk.addr;

    TRACE_RX(l1_trace(L1_TRACE_L1APP_RX_QEC_START, (uint32_t)in));

    rx_qec_correction(in, out, RX_ADC_DMA_SAMPLE_COUNT);

    pipe->qec.input.bytes_done += chunk.size;
    pipe->qec.output.bytes_done += chunk.size;

    TRACE_RX(l1_trace(L1_TRACE_L1APP_RX_QEC_COMP, (uint32_t)in));

    fifo_push(pipe->qec.output.fifo, &chunk);
}

// ddc2x4x.sx prototypes
extern void decimator_2x_8_Taps_asm(cfixed16_t *pOut, cfixed16_t *pIn, float32_t *pTaps, cfixed16_t *filtState, uint32_t n_samples);
extern void decimator_4x_8_Taps_asm(cfixed16_t *pOut, cfixed16_t *pIn, float32_t *pTaps, cfixed16_t *filtState, uint32_t n_samples);
static void decimate(rx_pipeline_t *pipe) {
    if (fifo_isfull(pipe->dec.output.fifo))
        return;

    MemoryBlock_t chunk;
    if (!fifo_pop(pipe->dec.input.fifo, &chunk))
        return;

    // in place
    cfixed16_t *in = chunk.addr;
    cfixed16_t *out = chunk.addr;

    TRACE_RX(l1_trace(L1_TRACE_L1APP_RX_DEC_START, (uint32_t)in));

    cfixed16_t *history = rx_decimation_history[pipe->channelIndex];

    switch (RX_CONFIG[pipe->channelIndex].oversample) {
    case 2:
        decimator_2x_8_Taps_asm(out, in, (float *)rx_filter_taps_downsampling, history, RX_ADC_DMA_SAMPLE_COUNT);
        break;
    case 4:
        decimator_4x_8_Taps_asm(out, in, (float *)rx_filter_taps_downsampling, history, RX_ADC_DMA_SAMPLE_COUNT);
        break;
    default:
        break;
    }

    pipe->dec.input.bytes_done += RX_DMA_TXR_STEP;
    pipe->dec.output.bytes_done += RX_DMA_TXR_STEP / RX_CONFIG[pipe->channelIndex].oversample;

    TRACE_RX(l1_trace(L1_TRACE_L1APP_RX_DEC_COMP, (uint32_t)in));
    fifo_push(pipe->dec.output.fifo, &chunk);
}

static void ddr_completion(rx_pipeline_t *pipe) {
    const uint32_t ddr_dma_mask = (1 << pipe->ddr_dma_channel);
    const uint32_t xfers_done = xfers_to_process(ddr_dma_mask, pipe->ddr.output.fifo);
    if (!xfers_done)
        return;

    uint32_t flags = PROXY_UPDATE_FLOW;
    for (uint32_t i = 0; i < xfers_done; ++i) {
        MemoryBlock_t chunk;
        if (!fifo_pop(pipe->ddr.output.fifo, &chunk))
            return;

        pipe->ddr.output.bytes_done += RX_DMA_TXR_STEP / RX_CONFIG[pipe->channelIndex].oversample;
        player_state.data_flow.rx[pipe->channelIndex].produced = pipe->ddr.output.bytes_done;

        TRACE_RX(l1_trace(L1_TRACE_MSG_DMA_DDR_WR_COMP, (uint32_t)chunk.addr));
        mempool_push(&vcpu_pool[pipe->channelIndex], &chunk);
        if ((pipe->ddr.output.bytes_done & (0xFFFFu)) == 0)
            flags |= PROXY_UPDATE_INTERRUPT; // interupt every 64KB
    }
    EnqueueProxyUpdate(flags);
}

static void ddr_enqueue(rx_pipeline_t *pipe) {
    const uint32_t ddr_dma_mask = (1 << pipe->ddr_dma_channel);
    if (dmac_is_available(ddr_dma_mask) != ddr_dma_mask)
        return;

    if (!RX_CONTROL[pipe->channelIndex].host_flow_control_disable) {
        const uint32_t ddr_filled = pipe->ddr.input.bytes_done - player_state.data_flow.rx[pipe->channelIndex].consumed;
        if (ddr_filled >= RX_CONFIG[pipe->channelIndex].ddr_size)
            return;
    }

    MemoryBlock_t chunk;
    if (!fifo_pop(pipe->ddr.input.fifo, &chunk))
        return;

    const uint32_t destaddr =
        RX_CONFIG[pipe->channelIndex].ddr_base_address + (pipe->ddr.input.bytes_done % (RX_CONFIG[pipe->channelIndex].ddr_size));
    const uint32_t xfer_size = RX_DMA_TXR_STEP / RX_CONFIG[pipe->channelIndex].oversample;
    DDR_write_multi_dma(pipe->ddr_dma_channel, 1, destaddr, VSPA_HALF_WORDS(chunk.addr), xfer_size);
    TRACE_RX(l1_trace(L1_TRACE_MSG_DMA_DDR_WR_START, (uint32_t)chunk.addr));

    pipe->ddr.input.bytes_done += xfer_size;
    fifo_push(pipe->ddr.output.fifo, &chunk);
}

// #pragma optimize_for_size off
void ProcessRx(void) {
    if (!RX_CONTROL[active_pipes[0]->channelIndex].ddr_enabled)
        goto end_rx_push;

    for (uint8_t c = 0; c < active_pipes_count; ++c)
        check_adc_axi_status(active_pipes[c]);

    for (uint8_t c = 0; c < active_pipes_count; ++c)
        adc_completion(active_pipes[c]);

    for (uint8_t c = 0; c < active_pipes_count; ++c)
        qec_work(active_pipes[c]);

    for (uint8_t c = 0; c < active_pipes_count; ++c) {
        if (RX_CONFIG[active_pipes[c]->channelIndex].oversample > 1)
            decimate(active_pipes[c]);
    }

    for (uint8_t c = 0; c < active_pipes_count; ++c)
        ddr_completion(active_pipes[c]);

    for (uint8_t c = 0; c < active_pipes_count; ++c)
        adc_enqueue(active_pipes[c]); // try to queue as soon as possible, if ddr_completion freed up a buffer

    for (uint8_t c = 0; c < active_pipes_count; ++c)
        ddr_enqueue(active_pipes[c]);

end_rx_push:
    // Check DMA errors
    const uint32_t tmp_dma_errors = iord(DMA_CFGERR_STAT);
    if (tmp_dma_errors != 0) {
        TRACE_RX(l1_trace(L1_TRACE_MSG_DMA_CFGERR, (uint32_t)tmp_dma_errors));
        // g_stats->gbl_stats[ERROR_DMA_CONFIG_ERROR]++;
        dmac_clear_errcfg();
    };

    // update host proxy if needed
    VSPA_PROXY_update();
}
#pragma optimize_for_size reset
