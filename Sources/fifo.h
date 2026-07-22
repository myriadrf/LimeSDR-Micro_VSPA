// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#ifndef LIME_FIFO_H
#define LIME_FIFO_H

#include <stdint.h>
#include <stdbool.h>

#define MFIFO_SIZE 4 // must be power of 2
#define MFIFO_SIZE_MASK (MFIFO_SIZE - 1)

struct MemoryFIFO {
    uint16_t items[MFIFO_SIZE];
    uint16_t head;
    uint16_t cnt;
};

static inline void fifo_reset(struct MemoryFIFO *fifo) {
    fifo->head = 0;
    fifo->cnt = 0;
}

static inline void fifo_push(struct MemoryFIFO *fifo, const uint16_t block) {
    fifo->items[(fifo->head + fifo->cnt) & MFIFO_SIZE_MASK] = block;
    ++fifo->cnt;
}

static inline uint16_t fifo_front(const struct MemoryFIFO *fifo) { return fifo->items[fifo->head]; }

static inline void fifo_pop(struct MemoryFIFO *fifo) {
    ++fifo->head;
    fifo->head &= MFIFO_SIZE_MASK;
    --fifo->cnt;
}

static inline uint16_t fifo_size(const struct MemoryFIFO *fifo) { return fifo->cnt; }

static inline bool fifo_isfull(const struct MemoryFIFO *fifo) { return fifo->cnt == MFIFO_SIZE; }

static inline bool fifo_isempty(const struct MemoryFIFO *fifo) { return fifo->cnt == 0; }

#endif // LIME_FIFO_H