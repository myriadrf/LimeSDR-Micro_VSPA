// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#ifndef LIME_FIFO_H
#define LIME_FIFO_H

#include "memory_pool.h"

struct MemoryFIFO {
    MemoryBlock_t items[8];
    uint16_t head;
    uint16_t tail;
};

void fifo_reset(struct MemoryFIFO *fifo);
bool fifo_push(struct MemoryFIFO *fifo, MemoryBlock_t *block);
bool fifo_pop(struct MemoryFIFO *fifo, MemoryBlock_t *block);
bool fifo_isfull(const struct MemoryFIFO *fifo);
uint32_t fifo_size(const struct MemoryFIFO *fifo);

#endif // LIME_FIFO_H