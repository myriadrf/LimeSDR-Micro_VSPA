// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#include "fifo.h"

void fifo_reset(struct MemoryFIFO *fifo) {
    fifo->head = 0;
    fifo->tail = 0;
}

bool fifo_push(struct MemoryFIFO *fifo, MemoryBlock_t *block) {
    if (fifo_isfull(fifo))
        return false;

    fifo->items[fifo->tail & 0x7] = *block;
    ++fifo->tail;
    return true;
}

bool fifo_pop(struct MemoryFIFO *fifo, MemoryBlock_t *block) {
    if (fifo->tail == fifo->head)
        return false;

    *block = fifo->items[fifo->head & 0x7];
    ++fifo->head;
    return true;
}

uint32_t fifo_size(const struct MemoryFIFO *fifo) { return fifo->tail - fifo->head; }

bool fifo_isfull(const struct MemoryFIFO *fifo) { return fifo_size(fifo) == (sizeof(fifo->items) / sizeof(MemoryBlock_t) - 1); }
