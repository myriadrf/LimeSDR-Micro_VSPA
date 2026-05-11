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

#include "receiver.h"
#include "tone_generator.h"

#define TX_DAC_DMA_SAMPLE_COUNT (512)
#define TX_DAC_BUF_COUNT 3
#define TX_INTERP_BUF_COUNT 4

#define TX_DMA_TXR_STEP (4 * TX_DAC_DMA_SAMPLE_COUNT)
#define DAC_AXI_FIFO_SIZE_BYTES 128

#define TX_DAC_DMA_MASK (0x1 << TX_DAC_WR_DMA_CHANNEL)

#define TX_CONFIG player_state.info.tx_config
#define TX_CONTROL player_state.internals.tx_control

#if 0
#define TRACE_TX(x) x
#else
#define TRACE_TX(x)
#endif

static tone_state_t tx_tone_state;

static cfixed16_t dac_buffer[TX_DAC_BUF_COUNT * TX_DAC_DMA_SAMPLE_COUNT] __attribute__((aligned(64), section(".vcpu_dmem")));
static cfixed16_t tx_ddr_buffer[TX_INTERP_BUF_COUNT * TX_DAC_DMA_SAMPLE_COUNT / 2]
    __attribute__((aligned(64), section(".vcpu_dmem")));

// separate pools for different sized blocks, in case of interpolation
static MemoryPool_t ddr_pool;
static MemoryPool_t dac_pool;

static const uint32_t dac_axi_fifo_addr = 0x4400B000; // axi_DAC_FIFO_addr

#define txpipe player_state.internals.txpipe

static void dac_finish(tx_pipeline_t *pipe);

static void TxAXIQ(bool enable) // for tracing pursposes
{
    if (enable) {
        l1_trace(L1_TRACE_MSG_TX_AXIQ, 1);
        axiq_tx_enable();
    } else {
        l1_trace(L1_TRACE_MSG_TX_AXIQ, 0);
        axiq_tx_disable();
    }
}

// PTR_RST must be done after:
// TX_DMA_Allowed(PHYTimer11) falling edge
// TX_AXIQ_Disable(), entered flush mode
void stream_write_ptr_rst(uint32_t dma_channel_wr, uint32_t axi_wr) {
    const uint32_t ctrl = DMAC_FIFO_RESET | DMAC_WRC | dma_channel_wr;
    const uint32_t vsp = VSPA_HALF_WORDS(dac_buffer);
    dmac_enable(ctrl, DAC_AXI_FIFO_SIZE_BYTES, axi_wr, vsp);
    l1_trace(L1_TRACE_MSG_DMA_PTR_RST, axi_wr);
}

static void stream_write_custom_size(uint32_t dma_channel_wr, uint32_t axi_wr, uint32_t vsp, uint32_t size_bytes) {
    uint32_t ctrl = DMAC_FIFO | DMAC_WRC | dma_channel_wr;
    dmac_enable(ctrl, size_bytes, axi_wr, vsp);
}

static void stream_write(uint32_t flags, uint32_t axi_wr, uint32_t vsp, uint32_t size_bytes) {
    const uint32_t ctrl = flags;
    dmac_enable(ctrl, size_bytes, axi_wr, vsp);
}

#define WAIT_TIMEOUT(cond, timeout_cycles) \
    do {                                   \
        uint32_t timeout = timeout_cycles; \
        do {                               \
        } while (!(cond) && --timeout);    \
    } while (0)

static void ResetDMA(void) {
    TxAXIQ(true);
    TxAXIQ(false); // enter flush mode

    WAIT_TIMEOUT(!dmac_is_enabled(TX_DAC_DMA_MASK), 100000);
    stream_write_ptr_rst(TX_DAC_WR_DMA_CHANNEL, dac_axi_fifo_addr); // stop flush mode
    WAIT_TIMEOUT(dmac_is_complete(TX_DAC_DMA_MASK), 100000);
    dmac_clear_complete(TX_DAC_DMA_MASK);
    // TxAXIQ(true);
}

void InitializeTx(void) {
    ResetDMA();
    TX_CONTROL.ddr_enabled = 0;
    TX_CONTROL.generate_tone = 0;
    TX_CONTROL.host_flow_control_disable = 0;
    TX_CONTROL.burst_fifo_offset = 0;
    TX_CONTROL.host_burst_size = 0;
    TX_CONTROL.burst_active = 0;

    TX_CONTROL.ddr_rd_dma_ch_nb = 0;
    TX_CONTROL.ddr_rd_dma_ch_mask = 0x0;
    TX_CONTROL.ddr_rd_dma_mBurst = 0;

    TX_CONFIG.oversample = 1;
    TX_CONFIG.host_fifo_address = 0xdeadbeef;
    TX_CONFIG.host_fifo_size = 0;
    TX_CONFIG.host_fifo_step = TX_DMA_TXR_STEP / TX_CONFIG.oversample;

    memclr(&player_state.data_flow.tx_issues, sizeof(player_state.data_flow.tx_issues));
    player_state.data_flow.tx.consumed = 0;
    player_state.data_flow.tx.produced = 0;

    tx_tone_state.amplitude = 0.9;
    tx_tone_state.phase = 0;
    tx_tone_state.freq_bin = 8192;

    dmac_clear_complete(TX_DAC_DMA_MASK);

    TxConfigure(TX_CONFIG.oversample);

    EnqueueProxyUpdate(PROXY_UPDATE_FLOW | PROXY_UPDATE_INFO);
}

#define TX_MAX_UPSAMPLE_TAPS 64
cfixed16_t tx_interpolation_history[TX_MAX_UPSAMPLE_TAPS - 1] __attribute__((aligned(64), section(".ippu_dmem"))) = { 0 };
// Interpolation function prototypes
void X2_interp_tap32_filter(__fx16 *output, __fx16 *input, unsigned int num_samples, __fx16 *history, float *filter_taps);
void X4_interp_tap64_filter(__fx16 *output, __fx16 *input, unsigned int num_samples, __fx16 *history, float *filter_taps);

static const float tx_filter_taps_upsampling_x2[32] __attribute__((aligned(64))) = {
#include "fir_interpolation_x2.txt"
};
// same coefficients as x2, just interleaved multiple instances.
static const float tx_filter_taps_upsampling_x4[128] __attribute__((aligned(64))) = {
#include "fir_interpolation_x4.txt"
};

static void interpolate(tx_pipeline_t *pipe) {
    if (fifo_isfull(pipe->interp.output.fifo))
        return;

    if (fifo_size(pipe->interp.input.fifo) == 0)
        return;

    MemoryBlock_t dac_chunk;
    if (!mempool_pop(&dac_pool, &dac_chunk))
        return;

    MemoryBlock_t ddr_chunk;
    if (!fifo_pop(pipe->interp.input.fifo, &ddr_chunk))
        return;

    cfixed16_t *in = ddr_chunk.addr;
    cfixed16_t *out = dac_chunk.addr;

    // l1_trace(L1_TRACE_L1APP_TX_INTERP_START, (uint32_t)in);

    const uint32_t in_samples_count = TX_DAC_DMA_SAMPLE_COUNT / TX_CONFIG.oversample;
    switch (TX_CONFIG.oversample) {
    case 2:
        X2_interp_tap32_filter((__fx16 *)out, (__fx16 *)in, in_samples_count, (__fx16 *)tx_interpolation_history,
                               (float *)tx_filter_taps_upsampling_x2);
        break;
    case 4:
        X4_interp_tap64_filter((__fx16 *)out, (__fx16 *)in, in_samples_count, (__fx16 *)tx_interpolation_history,
                               (float *)tx_filter_taps_upsampling_x4);
        break;
    default:
        break;
    }

    pipe->interp.input.bytes_done += TX_CONFIG.host_fifo_step;
    pipe->interp.output.bytes_done += TX_DMA_TXR_STEP;

    // l1_trace(L1_TRACE_L1APP_TX_INTERP_COMP, (uint32_t)in);
    fifo_push(pipe->interp.output.fifo, &dac_chunk);

    mempool_push(&ddr_pool, &ddr_chunk);
}

static void InitDAC_pool(MemoryPool_t *pool, cfixed16_t *mem_ptr) {
    mempool_clear(pool);
    for (uint32_t i = 0; i < TX_DAC_BUF_COUNT; ++i) {
        MemoryBlock_t block;
        block.addr = &mem_ptr[TX_DAC_DMA_SAMPLE_COUNT * (TX_DAC_BUF_COUNT - i - 1)];
        block.size = TX_DMA_TXR_STEP;
        mempool_push(pool, &block);
    }
}

static void InitDDR_pool(MemoryPool_t *pool, cfixed16_t *mem_ptr) {
    mempool_clear(pool);
    for (uint32_t i = 0; i < TX_INTERP_BUF_COUNT; ++i) {
        MemoryBlock_t block;
        block.addr = &mem_ptr[TX_DAC_DMA_SAMPLE_COUNT / TX_CONFIG.oversample * (TX_INTERP_BUF_COUNT - i - 1)];
        block.size = TX_CONFIG.host_fifo_step;
        mempool_push(pool, &block);
    }
}

static struct MemoryFIFO ddr_enq_fifo;
static struct MemoryFIFO ddr_ready_fifo;
static struct MemoryFIFO interp_out_fifo;
static struct MemoryFIFO qec_out_fifo;
static struct MemoryFIFO dac_enq_fifo;

static void pipeline_reset(tx_pipeline_t *pipe) {
    struct MemoryFIFO *last_stage_fifo = NULL;

    fifo_reset(&ddr_enq_fifo);
    fifo_reset(&ddr_ready_fifo);
    stage_setup(&pipe->ddr, &ddr_enq_fifo, &ddr_ready_fifo);
    last_stage_fifo = &ddr_ready_fifo;

    memclr(tx_interpolation_history, sizeof(tx_interpolation_history));
    fifo_reset(&interp_out_fifo);
    stage_setup(&pipe->interp, last_stage_fifo, &interp_out_fifo);
    if (TX_CONFIG.oversample > 1) {
        last_stage_fifo = &interp_out_fifo;
    }

    fifo_reset(&qec_out_fifo);
    stage_setup(&pipe->qec, last_stage_fifo, &qec_out_fifo);
    last_stage_fifo = &qec_out_fifo;

    fifo_reset(&dac_enq_fifo);
    stage_setup(&pipe->dac, last_stage_fifo, &dac_enq_fifo);
}

static void DDR_read_multi_dma(uint32_t DDR_rd_dma_channel, uint32_t nb_dma, uint32_t DDR_address, uint32_t vsp_address,
                               int32_t bytes_size) {
    uint32_t size = bytes_size / nb_dma;

    // user should ensure bytes_size/nb_dma is enire and aligned on AXI bus width 16B.
    // 2048B is ok with 1,2,4 dmas only
    // dma channel should be contiguous

    for (uint32_t i = 0; i < nb_dma; ++i) {
#pragma loop_count(1, 16, 2, 0)
        uint32_t ctrl = DMAC_RDC | (DDR_rd_dma_channel + i);
        if (TX_CONTROL.ddr_rd_dma_mBurst)
            ctrl |= DMAC_MBRE;
        dmac_enable(ctrl, size, DDR_address + i * size, vsp_address + i * size);
    }
}

void TxConfigureHostFIFO(uint32_t la9310_addr, uint32_t size) {
    l1_trace(L1_TRACE_MSG_TX_FIFO_SET, la9310_addr);
    TX_CONFIG.host_fifo_address = la9310_addr;
    TX_CONFIG.host_fifo_size = size;
}

lime_Result TxConfigure(uint32_t oversample) {
    l1_trace(L1_TRACE_MSG_TX_CONFIG, oversample);
    if (TX_CONTROL.ddr_enabled || TX_CONTROL.generate_tone)
        return lime_Result_Busy;

    TX_CONTROL.ddr_rd_dma_ch_nb = 1;

    // Burst=1, can get Tx DDR DMA stuck in running state and never complete, requiring power cycle to recover
    TX_CONTROL.ddr_rd_dma_mBurst = 0;
    TX_CONTROL.ddr_rd_dma_ch_mask = dma_chan_mask(DDR_RD_DMA_CHANNEL_1, TX_CONTROL.ddr_rd_dma_ch_nb);

    TX_CONTROL.burst_fifo_offset = 0;
    TX_CONTROL.host_burst_size = 0;
    TX_CONTROL.burst_active = 0;

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
    TX_CONFIG.host_fifo_step = TX_DMA_TXR_STEP / TX_CONFIG.oversample;

    player_state.data_flow.tx.consumed = 0;

    if (TX_CONFIG.oversample > 1)
        InitDDR_pool(&ddr_pool, tx_ddr_buffer);

    InitDAC_pool(&dac_pool, dac_buffer);
    pipeline_reset(&txpipe);
    memclr(&player_state.data_flow.tx_issues, sizeof(player_state.data_flow.tx_issues));

    EnqueueProxyUpdate(PROXY_UPDATE_FLOW | PROXY_UPDATE_INFO | PROXY_UPDATE_INTERNALS);
    return lime_Result_Success;
}

lime_Result TxDDR_control(uint64_t msg64) {
    const bool ddr_enable = (HIWORD(msg64)) & (1 << 0);

    l1_trace(L1_TRACE_MSG_TX_CONTROL, HIWORD(msg64));
    const uint32_t mask = TX_CONTROL.ddr_rd_dma_ch_mask | TX_DAC_DMA_MASK;
    if (TX_CONTROL.ddr_enabled != ddr_enable) {
        if (ddr_enable) {
            pipeline_reset(&txpipe);
            TX_CONTROL.ddr_enabled = true;
            TX_CONTROL.burst_fifo_offset = LOWORD(msg64);
            TX_CONTROL.burst_active = 1;
            TxSetBurstSize(0);

            TxAXIQ(true);
            // axiq_fifo_tx_clrerr(AXIQ_BANK_0, AXIQ_FIFO_TX0);
            axiq_fifo_tx_cr(AXIQ_BANK_0, AXIQ_FIFO_TX0, AXIQ_CR_CLRERR, AXIQ_CR_CLRERR);
            axiq_fifo_tx_cr(AXIQ_BANK_0, AXIQ_FIFO_TX0, AXIQ_CR_CLRERR, 0);

            dmac_clear_complete(mask);
            dmac_clear_event(mask);
            EnqueueProxyUpdate(PROXY_UPDATE_FLOW);
        } else {
            const uint32_t burst_length = LOWORD(msg64);
            if (burst_length > 0)
                TxSetBurstSize(burst_length); // stops Tx after transmission is done
            else {
                TxAXIQ(false); // enters DMA FLUSH mode
                TX_CONTROL.ddr_enabled = false; // stop immediately
                dac_finish(&txpipe); // wait for currently enqued transfers to complete
                // stream_write_ptr_rst() must be called to disable DMA FLUSH
                stream_write_ptr_rst(TX_DAC_WR_DMA_CHANNEL, dac_axi_fifo_addr);
                TX_CONTROL.burst_active = 0;
            }
            EnqueueProxyUpdate(PROXY_UPDATE_FLOW | PROXY_UPDATE_INFO | PROXY_UPDATE_INTERRUPT);
        }
    }

    return lime_Result_Success;
}

void TxTone_control(uint64_t msg64) {
    TX_CONTROL.generate_tone = HIWORD(msg64) & 0x00100000;
    // tx_tone_state.amplitude = (LOWORD(msg64) >> 16) / 32768.0;
    tx_tone_state.freq_bin = LOWORD(msg64) & 0xFFFF;
    // tx_tone_state.phase = 0;
}

static void check_dac_axi_status(const tx_pipeline_t *pipe) {
    const uint8_t field_shift = axiq_sr_shift(AXIQ_FIFO_TX0);
    // Check AXIQ tx fifo is not full or overrun
    const uint32_t status = axiq_fifo_tx_sr(AXIQ_BANK_0, AXIQ_FIFO_TX0, AXIQ_SR_FIELD_ERROVER | AXIQ_SR_FIELD_ERRUNDER);
    if (status == 0)
        return;

    if (status & (AXIQ_SR_FIELD_ERROVER << field_shift)) {
        ++player_state.data_flow.tx_issues.overrun;
        l1_trace_nr(L1_TRACE_MSG_DMA_AXIQ_TX_OVER, 0);
    }
    if (status & (AXIQ_SR_FIELD_ERRUNDER << field_shift)) {
        ++player_state.data_flow.tx_issues.underrun;
        l1_trace_nr(L1_TRACE_MSG_DMA_AXIQ_TX_UNDER, 0);
    }
    axiq_fifo_tx_cr(AXIQ_BANK_0, AXIQ_FIFO_TX0, AXIQ_CR_CLRERR, AXIQ_CR_CLRERR);
    axiq_fifo_tx_cr(AXIQ_BANK_0, AXIQ_FIFO_TX0, AXIQ_CR_CLRERR, 0);
}

static void ddr_completion(tx_pipeline_t *pipe) {
    const uint32_t xfers_done = xfers_to_process(TX_CONTROL.ddr_rd_dma_ch_mask, pipe->ddr.input.fifo);
    if (!xfers_done)
        return;

    // if (fifo_isfull(pipe->ddr.output.fifo))
    //     return;

    for (uint32_t i = 0; i < xfers_done; ++i) {
        MemoryBlock_t chunk;
        if (!fifo_pop(pipe->ddr.input.fifo, &chunk)) {
            break;
        }

        if (TX_CONTROL.generate_tone && TX_CONTROL.ddr_enabled) // replace received data with generated one
            gen_nco_single_tone(chunk.addr, TX_DAC_DMA_SAMPLE_COUNT / TX_CONFIG.oversample, &tx_tone_state);

        pipe->ddr.output.bytes_done += TX_CONFIG.host_fifo_step;
        fifo_push(pipe->ddr.output.fifo, &chunk);
        // l1_trace(L1_TRACE_MSG_DMA_DDR_RD_COMP, (uint32_t)chunk.addr);
    }
    player_state.data_flow.tx.consumed += TX_CONFIG.host_fifo_step;

    uint32_t proxy_flags = PROXY_UPDATE_FLOW;
    if (pipe->ddr.output.bytes_done & 0xFFFF == 0)
        proxy_flags |= PROXY_UPDATE_INTERRUPT; // trigger interrupt only every 64KB
    EnqueueProxyUpdate(proxy_flags);

    if (!dmac_errxfr(TX_CONTROL.ddr_rd_dma_ch_mask))
        return;

    dmac_clear_errxfr(TX_CONTROL.ddr_rd_dma_ch_mask);
    ++player_state.data_flow.tx_issues.xfer_errors;
    // l1_trace(L1_TRACE_MSG_DMA_AXIQ_TX_XFER_ERROR, player_state.data_flow.tx_issues.xfer_errors);
}

static void ddr_enqueue(tx_pipeline_t *pipe) {
    for (int i = 0; i < 2; ++i) {
        if (dmac_is_available(TX_CONTROL.ddr_rd_dma_ch_mask) != TX_CONTROL.ddr_rd_dma_ch_mask)
            return;

        if (TX_CONTROL.host_burst_size > 0 && TX_CONTROL.host_burst_size == pipe->ddr.input.bytes_done)
            return; // don't do more ddr transfers when Tx burst length has been reached

        if (!TX_CONTROL.host_flow_control_disable) {
            const uint32_t host_ddr_filled = player_state.data_flow.tx.produced - player_state.data_flow.tx.consumed;
            const bool overrun = host_ddr_filled > TX_CONFIG.host_fifo_size;
            if (overrun)
                ++player_state.data_flow.tx_issues.overrun;
            if (host_ddr_filled < TX_CONFIG.host_fifo_step || overrun)
                return;
        }

        MemoryBlock_t chunk;
        MemoryPool_t *mempool = TX_CONFIG.oversample > 1 ? &ddr_pool : &dac_pool; // TODO: set once during configuration
        if (!mempool_pop(mempool, &chunk))
            return;

        const uint32_t srcdata =
            TX_CONFIG.host_fifo_address + ((TX_CONTROL.burst_fifo_offset + pipe->ddr.input.bytes_done) % TX_CONFIG.host_fifo_size);
        const uint32_t dest = VSPA_HALF_WORDS(chunk.addr);
        const uint32_t xfer_size = TX_CONFIG.host_fifo_step;

        DDR_read_multi_dma(DDR_RD_DMA_CHANNEL_1, TX_CONTROL.ddr_rd_dma_ch_nb, srcdata, dest, xfer_size);

        // l1_trace(L1_TRACE_MSG_DMA_DDR_RD_START, (uint32_t)chunk.addr);
        pipe->ddr.input.bytes_done += xfer_size;

        fifo_push(pipe->ddr.input.fifo, &chunk);
    }
}

inline static void dac_completion(tx_pipeline_t *pipe) {
    const uint32_t xfers_done = xfers_to_process(TX_DAC_DMA_MASK, pipe->dac.output.fifo);
    if (!xfers_done)
        return;

    for (uint32_t i = 0; i < xfers_done; ++i) {
        MemoryBlock_t chunk;
        if (!fifo_pop(pipe->dac.output.fifo, &chunk))
            break;

        pipe->dac.output.bytes_done += TX_DMA_TXR_STEP;
        TRACE_TX(l1_trace(L1_TRACE_MSG_DMA_AXIQ_TX_COMP, (uint32_t)chunk.addr));

        if (TX_CONTROL.host_burst_size > 0 && (TX_CONTROL.host_burst_size * TX_CONFIG.oversample) == pipe->dac.output.bytes_done)
            TX_CONTROL.burst_active = 0;
        mempool_push(&dac_pool, &chunk);
    }

    EnqueueProxyUpdate(PROXY_UPDATE_FLOW);

    if (!dmac_errxfr(TX_DAC_DMA_MASK))
        return;

    dmac_clear_errxfr(TX_DAC_DMA_MASK);
    ++player_state.data_flow.tx_issues.xfer_errors;
    l1_trace(L1_TRACE_MSG_DMA_AXIQ_TX_XFER_ERROR, player_state.data_flow.tx_issues.xfer_errors);
}

static void dac_finish(tx_pipeline_t *pipe) {
    uint32_t timeout = 10000000;
    // need timeout just in case the DMA could be stuck waiting for trigger
    while ((dmac_is_enabled(TX_DAC_DMA_MASK) || fifo_size(pipe->dac.output.fifo)) && --timeout) {
        dac_completion(pipe);
    }
    EnqueueProxyUpdate(PROXY_UPDATE_FLOW | PROXY_UPDATE_INFO | PROXY_UPDATE_INTERRUPT);
}

inline static void dac_enqueue(tx_pipeline_t *pipe) {
    if (!TX_CONTROL.burst_active)
        return;

    for (uint32_t i = 0; i < 2; ++i) {
        if (!dmac_is_available(TX_DAC_DMA_MASK))
            return;

        MemoryBlock_t chunk;
        if (!fifo_pop(pipe->dac.input.fifo, &chunk))
            return;

        const uint32_t dac_xfer_size = TX_DMA_TXR_STEP;
        pipe->dac.input.bytes_done += dac_xfer_size;

        const uint32_t flags = DMAC_FIFO | DMAC_WRC | TX_DAC_WR_DMA_CHANNEL;
        // if (pipe->dac.input.bytes_done == TX_CONTROL.host_burst_size) {
        //     flags |= DMAC_FIFO_RESET | DMAC_TRIG_IRQ;
        //     l1_trace(L1_TRACE_MSG_DMA_PTR_RST, (uint32_t)chunk.addr);
        // }

        // DAC is 12bit, AXIQ does conversion 16bit -> 12bit, by rounding 4LSB and downshifting
        stream_write(flags, dac_axi_fifo_addr, VSPA_HALF_WORDS(chunk.addr), dac_xfer_size);
        // l1_trace(L1_TRACE_MSG_DMA_AXIQ_TX_START, (uint32_t)chunk.addr);

        fifo_push(pipe->dac.output.fifo, &chunk);
    }
}

static void qec_work(tx_pipeline_t *pipe) {
    if (fifo_isfull(pipe->qec.output.fifo))
        return;

    MemoryBlock_t chunk;
    if (!fifo_pop(pipe->qec.input.fifo, &chunk))
        return;

    // in place
    cfixed16_t *datain = chunk.addr;
    cfixed16_t *dataout = chunk.addr;

    // l1_trace(L1_TRACE_L1APP_TX_QEC_START, (uint32_t)datain);

    tx_qec_correction(datain, dataout, TX_DAC_DMA_SAMPLE_COUNT);

    pipe->qec.input.bytes_done += chunk.size;
    pipe->qec.output.bytes_done += chunk.size;

    // l1_trace(L1_TRACE_L1APP_TX_QEC_COMP, (uint32_t)datain);
    fifo_push(pipe->qec.output.fifo, &chunk);
}

static void tone_enqueue(tx_pipeline_t *pipe) {
    for (int i = 0; i < 2; ++i) {
        if (fifo_size(pipe->ddr.output.fifo) > 1)
            return; // avoid consuming all memory pool

        MemoryBlock_t chunk;
        MemoryPool_t *mempool = TX_CONFIG.oversample > 1 ? &ddr_pool : &dac_pool;
        if (!mempool_pop(mempool, &chunk))
            return;

        gen_nco_single_tone(chunk.addr, TX_DAC_DMA_SAMPLE_COUNT / TX_CONFIG.oversample, &tx_tone_state);
        fifo_push(pipe->ddr.output.fifo, &chunk);
    }
}

// #pragma optimize_for_size off
void ProcessTx(void) {
    check_dac_axi_status(&txpipe);
    if (!TX_CONTROL.ddr_enabled && !TX_CONTROL.generate_tone)
        goto end_tx_push;

    dac_completion(&txpipe);

    if (TX_CONTROL.host_burst_size > 0 && txpipe.dac.output.bytes_done == TX_CONTROL.host_burst_size * TX_CONFIG.oversample) {
        l1_trace(L1_TRACE_MSG_TX_BURST_END, (1 << 31) | TX_CONTROL.host_burst_size);
        TxDDR_control(0); // stop Tx
        return;
    }

    if (!TX_CONTROL.ddr_enabled && TX_CONTROL.generate_tone)
        tone_enqueue(&txpipe);
    else {
        ddr_completion(&txpipe);
        ddr_enqueue(&txpipe);
    }

    if (TX_CONFIG.oversample > 1)
        interpolate(&txpipe);

    qec_work(&txpipe);

    dac_enqueue(&txpipe);

end_tx_push:
    // Check DMA errors
    const uint32_t tmp_dma_errors = (uint32_t)iord(DMA_CFGERR_STAT);
    if (tmp_dma_errors != 0) {
        const uint32_t txmask = (1 << TX_DAC_WR_DMA_CHANNEL) | (1 << DDR_RD_DMA_CHANNEL_1) | (1 << DDR_RD_DMA_CHANNEL_2) |
                                (1 << DDR_RD_DMA_CHANNEL_3) | (1 << DDR_RD_DMA_CHANNEL_4);
        if (tmp_dma_errors & txmask)
            ++player_state.data_flow.tx_issues.xfer_config_errors;
        // g_stats->gbl_stats[ERROR_DMA_CONFIG_ERROR]++;
        l1_trace(L1_TRACE_MSG_DMA_CFGERR, tmp_dma_errors);
        dmac_clear_errcfg(txmask);
    };

    // update host proxy if needed
    VSPA_PROXY_update();
}

void TxSetBurstSize(uint32_t bytes) {
    TX_CONTROL.host_burst_size = bytes;
    // l1_trace(L1_TRACE_MSG_TX_SET_BURST_SIZE, TX_CONTROL_SET);
}
