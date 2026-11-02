// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#include "afe_pipeline.h"

void afe_axiq_fifo_enable(enum axiq_fifo_e fifo) {
    switch (fifo) {
    case AXIQ_FIFO_RX0:
    case AXIQ_FIFO_RX1:
    case AXIQ_FIFO_RX2:
    case AXIQ_FIFO_RX3:
        axiq_fifo_rx_cr(AXIQ_BANK_0, fifo, AXIQ_CR_FIELD_ENABLE, AXIQ_CR_ENABLE);
        return;
    case AXIQ_FIFO_TX0:
        axiq_fifo_tx_cr(AXIQ_BANK_0, fifo, AXIQ_CR_FIELD_ENABLE, AXIQ_CR_ENABLE);
        return;
    }
}

static inline void axiq_fifo_disable(enum axiq_fifo_e fifo) {
    switch (fifo) {
    case AXIQ_FIFO_RX0:
    case AXIQ_FIFO_RX1:
    case AXIQ_FIFO_RX2:
    case AXIQ_FIFO_RX3:
        axiq_fifo_rx_cr(AXIQ_BANK_0, fifo, AXIQ_CR_FIELD_ENABLE, AXIQ_CR_DISABLE);
        return;
    case AXIQ_FIFO_TX0:
        axiq_fifo_tx_cr(AXIQ_BANK_0, fifo, AXIQ_CR_FIELD_ENABLE, AXIQ_CR_DISABLE);
        return;
    }
}

static inline uint32_t axiq_fifo_status(enum axiq_fifo_e fifo) {
    switch (fifo) {
    case AXIQ_FIFO_RX0:
    case AXIQ_FIFO_RX1:
    case AXIQ_FIFO_RX2:
    case AXIQ_FIFO_RX3: {
        return axiq_fifo_rx_sr(AXIQ_BANK_0, fifo, AXIQ_SR_FIELD_ERROVER | AXIQ_SR_FIELD_ERRUNDER);
    }
    case AXIQ_FIFO_TX0: {
        return axiq_fifo_tx_sr(AXIQ_BANK_0, fifo, AXIQ_SR_FIELD_ERROVER | AXIQ_SR_FIELD_ERRUNDER);
    }
    }
    return 0;
}

void afe_axiq_fifo_clear_err(enum axiq_fifo_e fifo) {
    switch (fifo) {
    case AXIQ_FIFO_RX0:
    case AXIQ_FIFO_RX1:
    case AXIQ_FIFO_RX2:
    case AXIQ_FIFO_RX3: {
        axiq_fifo_rx_cr(AXIQ_BANK_0, fifo, AXIQ_CR_CLRERR, AXIQ_CR_CLRERR);
        axiq_fifo_rx_cr(AXIQ_BANK_0, fifo, AXIQ_CR_CLRERR, 0);
    }
    case AXIQ_FIFO_TX0: {
        axiq_fifo_tx_cr(AXIQ_BANK_0, fifo, AXIQ_CR_CLRERR, AXIQ_CR_CLRERR);
        axiq_fifo_tx_cr(AXIQ_BANK_0, fifo, AXIQ_CR_CLRERR, 0);
    }
    }
}

void afe_axiq_fifo_reset(afe_pipeline_t *pipe) {
    afe_axiq_fifo_enable(pipe->config.axi_fifo_index);
    const uint32_t dma_mask = (1 << afe_dma_channel(pipe));
    dmac_abort(dma_mask);

    axiq_fifo_disable(pipe->config.axi_fifo_index); // falling edge enters flush mode
    timer_trig_immediate(pipe->config.phytimer_id, ePhyTimerComparatorOut1);
    // wait for DMA to completely stop before issuing new commands
    WAIT_FOR(!dmac_is_running(dma_mask), VSPA_DEFAULT_TIMEOUT);

    // fifo ptr_rst, exits flush mode
    dmac_enable(pipe->config.dmac_ctrl_flags | DMAC_PRST_REQ, 16, pipe->config.axi_fifo_addr, VCPU_ADDR_FOR_DMA(pipe->base_buffer));

    WAIT_FOR(dmac_is_complete(dma_mask), VSPA_DEFAULT_TIMEOUT);
    timer_trig_immediate(pipe->config.phytimer_id, ePhyTimerComparatorOut0);
    dma_clear_all_bits(dma_mask);
    afe_axiq_fifo_clear_err(pipe->config.axi_fifo_index);
}

void afe_pipe_reset(afe_pipeline_t *pipe, cfixed16_t *buffer) {
    pipe->base_buffer = buffer;
    pipe->count_dmac_enque = 0;
    pipe->count_dmac_complete = 0;
    pipe->buffers_consumed = 0;

    afe_axiq_fifo_reset(pipe);
}

void afe_pipe_abort(afe_pipeline_t *pipe) {
    const uint32_t dma_mask = 1 << afe_dma_channel(pipe);
    axiq_fifo_disable(pipe->config.axi_fifo_index); // enter DMA flush mode
    dmac_abort(dma_mask);
    WAIT_FOR(!dmac_is_running(dma_mask), VSPA_DEFAULT_TIMEOUT);
    timer_trig_immediate(pipe->config.phytimer_id, ePhyTimerComparatorOut0);
    dmac_clear_complete(dma_mask);
    dmac_clear_event(dma_mask);
}

// @return true if AXIQ FIFO has overrun, and samples were lost
bool axiq_fifo_errors(afe_pipeline_t *pipe) {
    const uint8_t field_shift = axiq_sr_shift(pipe->config.axi_fifo_index);

    // Check AXIQ rx fifo is not full or overrun
    uint32_t status = axiq_fifo_status(pipe->config.axi_fifo_index);
    if (status == 0)
        return false;

    status >>= field_shift;
    if (status & AXIQ_SR_FIELD_ERROVER) {
        ++pipe->ovr;
    }
    if (status & AXIQ_SR_FIELD_ERRUNDER) {
        ++pipe->udr;
    }
    afe_axiq_fifo_clear_err(pipe->config.axi_fifo_index);
    return status;
}
