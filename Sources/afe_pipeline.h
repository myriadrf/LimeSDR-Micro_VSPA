// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#ifndef LIME_VSPA_AFE_PIPELINE_H
#define LIME_VSPA_AFE_PIPELINE_H

#include "vcpu.h"

#include "vspa_iqstream.h"
#include "dma_common.h"
#include "axiq-la9310.h"

#include "phytimer.h"
#include "vspa_memorymap.h"

#include <stdint.h>
#include <stdbool.h>

#define AXIQ_XFER_SAMPLES_COUNT 512

#define RO0_AXI_FIFO_ADDR 0x44001000
#define RO1_AXI_FIFO_ADDR 0x44002000
#define RX0_AXI_FIFO_ADDR 0x44003000
#define RX1_AXI_FIFO_ADDR 0x44004000
#define TX0_AXI_FIFO_ADDR 0x4400B000

// #define AXIQ_FIFO_BEAT_COUNT 16
// #define addr_beat_offset (0x1000 - AXIQ_FIFO_BEAT_COUNT*16)
#define addr_beat_offset 0

// AXIQ FIFO <-> DMEM transfer configuration of a pipe, set when the pipe's channel is selected
typedef struct AXIQ_Config {
    uint32_t dmac_ctrl_flags; // DMA transfer control: direction, conversion, handshake, dma_channel
    uint32_t axi_fifo_addr; // AXIQ FIFO address on the AXI bus
    uint16_t dma_channel; // VSPA DMA channel moving the samples between the AXIQ FIFO and DMEM
    uint16_t phytimer_id; // PHY timer comparator gating the FIFO (rx/tx allowed)
    uint16_t cr_shift;
    uint16_t cr_index;
    uint16_t sr_shift;
    uint16_t sr_index;
} axiq_config_t;

#define AXIQ_CONFIG_RX(fifo_addr, dma_ch, timer, fifo) \
    { DMAC_FIFO_HS | DMAC_RDC | (dma_ch), fifo_addr, dma_ch, timer }
#define AXIQ_CONFIG_TX(fifo_addr, dma_ch, timer, fifo) \
    { DMAC_FIFO_HS | DMAC_WRC | (dma_ch), fifo_addr, dma_ch, timer, AXIQ_CR_SHIFT_TX0, AXIQ_CR3, AXIQ_SR_SHIFT_TX0, AXIQ_SR1 }

// Analog front end state for moving data between DMEM and ADC/DAC AXIQ
typedef struct AFE_pipeline {
    cfixed16_t *const base_buffer;
    const axiq_config_t *const config; // the pipe's entry of rx/tx_axiq_configs[]
    uint32_t count_dmac_enque;
    uint32_t count_dmac_complete;
    union {
        // naming convenience depending on pipeline direction
        uint32_t buffers_prepared;
        uint32_t buffers_consumed;
    };
    uint16_t ovr;
    uint16_t udr;
} afe_pipeline_t;

// AXIQ configuration of an Rx channel (e_rx_channel): RO0, RO1, RX0, RX1 have consecutive FIFOs, ADC DMA channels and
// rx_allowed PHY timer comparators.
static inline void rx_axiq_fill_config(axiq_config_t *cfg, uint16_t channel) {
    cfg->axi_fifo_addr = RO0_AXI_FIFO_ADDR + channel * 0x1000;
    cfg->dma_channel = RO0_ADC_RD_DMA_CHANNEL + channel;
    cfg->phytimer_id = PHY_TIMER_COMP_CH1_RX_ALLOWED + channel;
    // cfg->axi_fifo_index = AXIQ_FIFO_RX0 + channel;
    cfg->dmac_ctrl_flags = DMAC_FIFO_HS | DMAC_RDC | (RO0_ADC_RD_DMA_CHANNEL + channel);
    cfg->cr_index = AXIQ_CR0; // LA9310 same for all Rx channels
    cfg->sr_index = AXIQ_SR0; // LA9310 same for all Rx channels
    cfg->cr_shift = AXIQ_CR_SHIFT_RX0 + (channel * 8);
    cfg->sr_shift = AXIQ_SR_SHIFT_RX0 + (channel * 4);
}

static inline uint16_t afe_dma_channel(const afe_pipeline_t *pipe) { return pipe->config->dma_channel; }

// @return true if AXIQ FIFO has overrun/underrun
bool axiq_fifo_errors(afe_pipeline_t *pipe);

void afe_pipe_reset(afe_pipeline_t *pipe);
void afe_pipe_abort(afe_pipeline_t *pipe);

static inline uint32_t phytime_now(void) { return gpird(4); }

static inline cfixed16_t *afe_next_enq_buffer(afe_pipeline_t *pipe) {
    return pipe->base_buffer + (pipe->count_dmac_enque & 0x1) * AXIQ_XFER_SAMPLES_COUNT;
}

static inline cfixed16_t *afe_next_complete_buffer(afe_pipeline_t *pipe) {
    return pipe->base_buffer + (pipe->count_dmac_complete & 0x1) * AXIQ_XFER_SAMPLES_COUNT;
}

static inline uint16_t afe_buffers_in_flight(const afe_pipeline_t *pipe) {
    return pipe->count_dmac_enque - pipe->count_dmac_complete;
}

static inline uint16_t afe_dac_buffers_prepared(const afe_pipeline_t *pipe) {
    return pipe->buffers_prepared - pipe->count_dmac_enque;
}

static inline uint16_t afe_dac_buffers_in_use(const afe_pipeline_t *pipe) {
    return pipe->buffers_prepared - pipe->count_dmac_complete;
}

static inline cfixed16_t *afe_dac_next_free_buffer(const afe_pipeline_t *pipe) {
    return pipe->base_buffer + (pipe->buffers_prepared & 0x1) * AXIQ_XFER_SAMPLES_COUNT;
}

#endif // LIME_VSPA_AFE_PIPELINE_H
