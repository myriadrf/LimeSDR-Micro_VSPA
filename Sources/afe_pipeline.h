// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#ifndef LIME_VSPA_AFE_PIPELINE_H
#define LIME_VSPA_AFE_PIPELINE_H

#include "vcpu.h"

#include "vspa_iqstream.h"
#include "dma_common.h"
#include "axiq-la9310.h"

#include "phytimer.h"

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

// Non changing data
typedef struct AXIQ_Config {
    uint32_t axi_fifo_addr;
    uint16_t dma_channel;
    uint16_t phytimer_id;
    enum axiq_fifo_e axi_fifo_index;
    uint32_t dmac_ctrl_flags; // dma channel, direction, conversion
} axiq_config_t;

// Analog front end state for moving data between DMEM and ADC/DAC AXIQ
typedef struct AFE_pipeline {
    cfixed16_t *base_buffer;
    axiq_config_t config;
    uint32_t count_dmac_enque;
    uint32_t count_dmac_complete;
    uint16_t ovr;
    uint16_t udr;
    union {
        // naming convenience depending on pipeline direction
        uint32_t buffers_prepared;
        uint32_t buffers_consumed;
    };
} afe_pipeline_t;

static inline void rx_axiq_fill_config(axiq_config_t *cfg, uint16_t channel) {
    cfg->axi_fifo_addr = RO0_AXI_FIFO_ADDR + (channel * 0x1000); // + addr_beat_offset;
    cfg->axi_fifo_index = (enum axiq_fifo_e)(AXIQ_FIFO_RX0 + channel);
    cfg->dma_channel = RO0_ADC_RD_DMA_CHANNEL + channel;
    cfg->phytimer_id = PHY_TIMER_COMP_CH1_RX_ALLOWED + channel;
    cfg->dmac_ctrl_flags = DMAC_FIFO_HS | DMAC_RDC | RO0_ADC_RD_DMA_CHANNEL + channel;
}

static inline void tx_axiq_fill_config(axiq_config_t *cfg, uint16_t channel) {
    cfg->axi_fifo_addr = TX0_AXI_FIFO_ADDR; // + addr_beat_offset;
    cfg->axi_fifo_index = AXIQ_FIFO_TX0;
    cfg->dma_channel = TX_DAC_WR_DMA_CHANNEL;
    cfg->phytimer_id = PHY_TIMER_COMP_CH5_TX_ALLOWED;
    cfg->dmac_ctrl_flags = DMAC_FIFO_HS | DMAC_WRC | TX_DAC_WR_DMA_CHANNEL;
}

static inline uint16_t afe_dma_channel(const afe_pipeline_t *pipe) { return pipe->config.dma_channel; }

void afe_axiq_fifo_enable(enum axiq_fifo_e fifo);
void afe_axiq_fifo_clear_err(enum axiq_fifo_e fifo);
void afe_axiq_fifo_reset(afe_pipeline_t *pipe);

// @return true if AXIQ FIFO has overrun, and samples were lost
bool axiq_fifo_errors(afe_pipeline_t *pipe);

void afe_pipe_reset(afe_pipeline_t *pipe, cfixed16_t *buffer);
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
