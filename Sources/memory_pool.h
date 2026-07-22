// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <stdint.h>
#include <stdbool.h>

#define MEM_POOL_SIZE 8

typedef struct MemoryBlock {
    void *addr;
    uint32_t size;
    uint32_t timestamp;
    uint32_t flags;
} MemoryBlock_t;

typedef uint16_t MetaHandle_t;

#define INVALID_HANDLE 0xFFFF

typedef struct HandlesStack {
    MetaHandle_t items[MEM_POOL_SIZE];
    uint16_t count;
} HandlesStack_t;

static inline void handles_stack_clear(HandlesStack_t *pool) { pool->count = 0; }

static inline void handles_stack_push(HandlesStack_t *pool, const MetaHandle_t h) {
    pool->items[pool->count] = h;
    ++pool->count;
}

static inline void handles_stack_pop(HandlesStack_t *pool) { --pool->count; }

static inline MetaHandle_t handles_stack_top(HandlesStack_t *pool) { return pool->items[pool->count - 1]; }

static inline MetaHandle_t handles_stack_isempty(HandlesStack_t *pool) { return pool->count == 0; }

#endif
