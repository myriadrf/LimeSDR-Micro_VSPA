// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#ifndef LIME_DMA_TABLE_H
#define LIME_DMA_TABLE_H

#include <stdint.h>
#include <stdbool.h>

#define DMA_TABLE_LINES 32 // expected to be power of 2
#define DMA_TABLE_LINES_MASK (DMA_TABLE_LINES - 1)

typedef struct DMA_LINE {
    uint64_t timestamp;
    uint32_t addr;
    uint32_t size;
    uint32_t flags;
} dma_line_t;

typedef struct DMA_TABLE {
    dma_line_t items[DMA_TABLE_LINES];
    uint16_t head;
    uint16_t cnt;
} dma_table_t;

static inline void dma_table_reset(dma_table_t *fifo) {
    fifo->head = 0;
    fifo->cnt = 0;
}

static inline void dma_table_push(dma_table_t *fifo, const dma_line_t *block) {
    fifo->items[(fifo->head + fifo->cnt) & DMA_TABLE_LINES_MASK] = *block;
    ++fifo->cnt;
}

static inline dma_line_t *dma_table_front(dma_table_t *fifo) { return &fifo->items[fifo->head]; }

static inline void dma_table_pop(dma_table_t *fifo) {
    ++fifo->head;
    fifo->head &= DMA_TABLE_LINES_MASK;
    --fifo->cnt;
}

static inline uint16_t dma_table_size(const dma_table_t *fifo) { return fifo->cnt; }

static inline bool dma_table_isfull(const dma_table_t *fifo) { return fifo->cnt == DMA_TABLE_LINES_MASK; }

static inline bool dma_table_isempty(const dma_table_t *fifo) { return fifo->cnt == 0; }

#endif // LIME_DMA_TABLE_H
