// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#ifndef LIME_DMA_TCD_FIFO_H
#define LIME_DMA_TCD_FIFO_H

#include "vspa_dma_hif.h"

#include <stdint.h>
#include <stdbool.h>

#define MFIFO_SIZE 32 // must be power of 2
#define MFIFO_SIZE_MASK (MFIFO_SIZE - 1)

typedef struct TCD_FIFO {
    vspa_dma_tcd_t items[MFIFO_SIZE];
    uint16_t head;
    uint16_t cnt;
} dma_tcd_fifo_t;

static inline void tcd_fifo_reset(dma_tcd_fifo_t *fifo) {
    fifo->head = 0;
    fifo->cnt = 0;
}

static inline void tcd_fifo_push(dma_tcd_fifo_t *fifo, vspa_dma_tcd_t block) {
    fifo->items[(fifo->head + fifo->cnt) & MFIFO_SIZE_MASK] = block;
    ++fifo->cnt;
}

static inline vspa_dma_tcd_t *tcd_fifo_front(dma_tcd_fifo_t *fifo) { return &fifo->items[fifo->head]; }

static inline void tcd_fifo_pop(dma_tcd_fifo_t *fifo) {
    ++fifo->head;
    fifo->head &= MFIFO_SIZE_MASK;
    --fifo->cnt;
}

static inline uint16_t tcd_fifo_size(const dma_tcd_fifo_t *fifo) { return fifo->cnt; }

static inline bool tcd_fifo_isfull(const dma_tcd_fifo_t *fifo) { return fifo->cnt == MFIFO_SIZE; }

static inline bool tcd_fifo_isempty(const dma_tcd_fifo_t *fifo) { return fifo->cnt == 0; }

#endif // LIME_DMA_TCD_FIFO_H