// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <stdint.h>
#include <stdbool.h>

typedef struct MemoryBlock {
    void *addr;
    uint32_t size;
} MemoryBlock_t;

typedef struct MemoryPool {
    MemoryBlock_t items[8];
    uint32_t count;
} MemoryPool_t;

void mempool_clear(MemoryPool_t *pool);
bool mempool_push(MemoryPool_t *pool, const MemoryBlock_t *block);
bool mempool_pop(MemoryPool_t *pool, MemoryBlock_t *block);

#endif
