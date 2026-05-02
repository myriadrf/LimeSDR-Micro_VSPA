// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#include "memory_pool.h"

void mempool_clear(MemoryPool_t *pool) { pool->count = 0; }

bool mempool_push(MemoryPool_t *pool, const MemoryBlock_t *block) {
    if (pool->count >= 8)
        return false;

    pool->items[pool->count] = *block;
    ++pool->count;
    return true;
}

bool mempool_pop(MemoryPool_t *pool, MemoryBlock_t *block) {
    if (pool->count == 0)
        return false;

    *block = pool->items[pool->count - 1];
    --pool->count;
    return true;
}
