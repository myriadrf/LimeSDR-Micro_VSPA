// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#ifndef LIME_DDR_PIPELINE_H
#define LIME_DDR_PIPELINE_H

#include "vcpu.h"
#include "dma_common.h"

#include <stdint.h>
#include <stdbool.h>

#define DDR_XFER_SAMPLES_COUNT 512

typedef struct MetaData {
    uint64_t timestamp; // first sample's timestamp, samples count since stream start (after decimation)
    uint32_t size; // bytes of contiguous samples written into the TCD buffer
    uint32_t flags;
} meta_t;

typedef struct DDR_pipeline {
    vspa_dma_hif_t dma;
    cfixed16_t *base_buffer;
    uint32_t count_dmac_enque;
    uint32_t count_dmac_complete;
    union {
        // depending on pipeline direction
        uint32_t buffers_prepared;
        uint32_t buffers_consumed;
    };
    uint16_t dma_channel;
    meta_t meta[2];
    dma_tcd_t wip_tcd; // working copy of input tcd, so the original could be recirculated.
} ddr_pipeline_t;

static void ddr_pipe_reset(ddr_pipeline_t *ddr, cfixed16_t *buffer) {
    ddr->base_buffer = buffer;
    ddr->count_dmac_enque = 0;
    ddr->count_dmac_complete = 0;
    ddr->buffers_consumed = 0;
}

static inline cfixed16_t *ddr_next_enq_buffer(const ddr_pipeline_t *pipe) {
    return pipe->base_buffer + (pipe->count_dmac_enque & 0x1) * DDR_XFER_SAMPLES_COUNT;
}

// DDR transfers in flight on the DMA channel
static inline uint16_t ddr_xfer_in_flight(const ddr_pipeline_t *pipe) { return pipe->count_dmac_enque - pipe->count_dmac_complete; }

static inline meta_t *ddr_next_enq_meta(ddr_pipeline_t *pipe) { return &pipe->meta[pipe->count_dmac_enque & 0x1]; }

static inline meta_t *ddr_next_complete_meta(ddr_pipeline_t *pipe) { return &pipe->meta[pipe->count_dmac_complete & 0x1]; }

static inline uint16_t ddr_tx_buffers_in_use(const ddr_pipeline_t *pipe) { return pipe->count_dmac_enque - pipe->buffers_consumed; }
static inline uint16_t ddr_tx_buffers_ready(const ddr_pipeline_t *pipe) {
    return pipe->count_dmac_complete - pipe->buffers_consumed;
}

#endif // LIME_DDR_PIPELINE_H
