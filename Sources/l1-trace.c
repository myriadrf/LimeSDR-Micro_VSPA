/* SPDX-License-Identifier: BSD-3-Clause */

/*
 * Copyright 2024 NXP
 */

#include "l1-trace.h"

l1_trace_data_t l1_trace_data[L1_TRACE_COUNT] __attribute__((aligned(64), section(".ippu_dmem")));
static uint32_t l1_trace_index = 0;
volatile uint32_t l1_trace_disable = 0;

#if L1_TRACE

void l1_trace_clear(void) {
    for (int i = 0; i < L1_TRACE_COUNT; i++) {
        l1_trace_data[i].cnt = 0;
        l1_trace_data[i].msg = 0;
        l1_trace_data[i].param = 0;
    }
    l1_trace_index = 0;
}

void l1_trace_dma(uint16_t dma_mask, uint32_t msg, uint32_t param) {
    if (l1_trace_disable)
        return;

    l1_trace_data[l1_trace_index].cnt = ccnt_read();
    l1_trace_data[l1_trace_index].msg = msg; // | ((uint32_t)dma_mask << 16);
    l1_trace_data[l1_trace_index].param = param;
    l1_trace_index++;

    if (l1_trace_index >= L1_TRACE_COUNT) {
        l1_trace_index = 0;
    }
}

#pragma cplusplus on

void l1_trace(uint32_t msg, uint32_t param) {
    if (l1_trace_disable)
        return;

    l1_trace_data[l1_trace_index].cnt = ccnt_read();
    l1_trace_data[l1_trace_index].msg = msg;
    l1_trace_data[l1_trace_index].param = param;
    l1_trace_index++;

    if (l1_trace_index >= L1_TRACE_COUNT) {
        l1_trace_index = 0;
    }
}

// no repeat version
void l1_trace_nr(uint32_t msg, uint32_t param) {
    static uint32_t prev_index = 0xFFF, new_index = 0xFFF;

    if (l1_trace_disable)
        return;

    if ((new_index == l1_trace_index) && (msg == l1_trace_data[prev_index].msg) && (param == l1_trace_data[prev_index].param)) {
        return;
    } else {
        prev_index = l1_trace_index;

        l1_trace_data[l1_trace_index].cnt = ccnt_read();
        l1_trace_data[l1_trace_index].msg = msg;
        l1_trace_data[l1_trace_index].param = param;
        l1_trace_index++;

        if (l1_trace_index >= L1_TRACE_COUNT) {
            l1_trace_index = 0;
        }

        new_index = l1_trace_index;
    }
}

#endif

#pragma cplusplus reset
