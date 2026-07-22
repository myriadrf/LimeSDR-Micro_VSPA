// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#ifndef PIPELINES_H
#define PIPELINES_H

#include <stdint.h>

#include "receiver.h"
#include "memory_pool.h"

#ifndef __VSPA__ // pointer wrapper to maintain struct size/layout when accessing from other platforms
#define VSPA_PTR(x) uint32_t
#else
#include "fifo.h"
#define VSPA_PTR(x) x
#endif

typedef struct stage_dir {
    VSPA_PTR(struct MemoryFIFO *) fifo;
    uint32_t bytes_done;
} stage_dir_t;

typedef struct Stage {
    stage_dir_t input;
    stage_dir_t output;
} stage_t;

typedef struct rx_pipeline {
    e_rx_channel channelIndex;
    uint32_t adc_axi_fifo_addr;
    uint16_t adc_dma_channel;
    uint16_t ddr_dma_channel;
    struct Stage adc;
    struct Stage ddr;
    HandlesStack_t mem_handles_pool;
} rx_pipeline_t;

typedef struct tx_pipeline {
    struct Stage ddr;
    struct Stage interp;
    struct Stage dac;
} tx_pipeline_t;

// Packet flags
enum {
    PKT_HAS_TIMESTAMP = (1 << 0),
    PKT_START = (1 << 1),
    PKT_END = (1 << 2),
    PKT_IRQ = (1 << 3),
    PKT_DMA_TCD_END = (1 << 4),
};

static inline void stage_setup(stage_t *s, struct MemoryFIFO *inputpool, struct MemoryFIFO *outputpool) {
    s->input.fifo = inputpool;
    s->output.fifo = outputpool;
    s->input.bytes_done = 0;
    s->output.bytes_done = 0;
}

#endif
