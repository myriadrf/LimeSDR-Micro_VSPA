// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#ifndef PIPELINES_H
#define PIPELINES_H

#include <stdint.h>

#include "receiver.h"

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
    uint32_t adc_dma_channel;
    uint32_t ddr_dma_channel;
    struct Stage adc;
    struct Stage qec;
    struct Stage dec;
    struct Stage ddr;
} rx_pipeline_t;

typedef struct tx_pipeline {
    struct Stage ddr;
    struct Stage interp;
    struct Stage qec;
    struct Stage dac;
} tx_pipeline_t;

void stage_setup(stage_t *s, struct MemoryFIFO *inputpool, struct MemoryFIFO *outputpool);

#endif
