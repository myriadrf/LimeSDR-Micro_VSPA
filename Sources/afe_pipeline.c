// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#include "afe_pipeline.h"

static inline bool axiq_fifo_is_tx(enum axiq_fifo_e fifo) { return fifo == AXIQ_FIFO_TX0; }

static inline void axiq_pipe_clear_err(const axiq_config_t *axiq) {
    iowr(axiq->cr_index, (AXIQ_CR_FIELD_FIFO & AXIQ_CR_CLRERR) << axiq->cr_shift, AXIQ_CR_CLRERR << axiq->cr_shift);
    iowr(axiq->cr_index, (AXIQ_CR_FIELD_FIFO & AXIQ_CR_CLRERR) << axiq->cr_shift, 0);
}

void axiq_pipe_enable(const axiq_config_t *axiq) {
    iowr(axiq->cr_index, (AXIQ_CR_FIELD_FIFO & AXIQ_CR_FIELD_ENABLE) << axiq->cr_shift, AXIQ_CR_ENABLE << axiq->cr_shift);
    // Enabling Rx AXIQ instantly generates underrun error, clear it.
    axiq_pipe_clear_err(axiq);
}

static inline void axiq_fifo_disable(const axiq_config_t *axiq) {
    iowr(axiq->cr_index, (AXIQ_CR_FIELD_FIFO & AXIQ_CR_FIELD_ENABLE) << axiq->cr_shift, AXIQ_CR_DISABLE << axiq->cr_shift);
}

static inline uint32_t axiq_fifo_status(const axiq_config_t *axiq) {
    return iord(axiq->sr_index, (AXIQ_SR_FIELD_FIFO & AXIQ_SR_FIELD_ERROVER | AXIQ_SR_FIELD_ERRUNDER) << axiq->sr_shift);
}

void axiq_pipe_reset(afe_pipeline_t *pipe) {
    const uint32_t dma_mask = (1 << afe_dma_channel(pipe));
    axiq_pipe_enable(pipe->config);
    dmac_abort(dma_mask);

    axiq_fifo_disable(pipe->config); // falling edge enters flush mode
    timer_trig_immediate_async(pipe->config->phytimer_id, ePhyTimerComparatorOut1);
    // wait for DMA to completely stop before issuing new commands
    WAIT_FOR(!dmac_is_running(dma_mask), VSPA_DEFAULT_TIMEOUT);

    // fifo ptr_rst, exits flush mode
    dmac_enable(pipe->config->dmac_ctrl_flags | DMAC_PRST_REQ, 16, pipe->config->axi_fifo_addr,
                VCPU_ADDR_FOR_DMA(pipe->base_buffer));

    WAIT_FOR(dmac_is_complete(dma_mask), VSPA_DEFAULT_TIMEOUT);
    timer_trig_immediate_async(pipe->config->phytimer_id, ePhyTimerComparatorOut0);
    // timer_wait_done();
    dma_clear_all_bits(dma_mask);
    // axiq_pipe_clear_err(pipe->config); // will be cleared on next axiq enable
}

void afe_pipe_reset(afe_pipeline_t *pipe) {
    pipe->count_dmac_enque = 0;
    pipe->count_dmac_complete = 0;
    pipe->buffers_consumed = 0;

    axiq_pipe_reset(pipe);
}

void afe_pipe_abort(afe_pipeline_t *pipe) {
    const uint32_t dma_mask = 1 << afe_dma_channel(pipe);
    axiq_fifo_disable(pipe->config); // enter DMA flush mode
    dmac_abort(dma_mask);
    WAIT_FOR(!dmac_is_running(dma_mask), VSPA_DEFAULT_TIMEOUT);
    timer_trig_immediate_async(pipe->config->phytimer_id, ePhyTimerComparatorOut0);
    dmac_clear_complete(dma_mask);
    dmac_clear_event(dma_mask);
}

// @return true if AXIQ FIFO has overrun, and samples were lost
bool axiq_fifo_errors(afe_pipeline_t *pipe) {
    // Check AXIQ rx fifo is not full or overrun
    uint32_t status = axiq_fifo_status(pipe->config);
    if (status == 0)
        return false;

    status >>= pipe->config->sr_shift;
    if (status & (AXIQ_SR_FIELD_ERROVER | AXIQ_SR_FIELD_ERRUNDER)) {
        ++pipe->ovr;
    }
    if (status & AXIQ_SR_FIELD_ERRUNDER) {
        ++pipe->udr;
    }
    axiq_pipe_clear_err(pipe->config);
    return true;
}
