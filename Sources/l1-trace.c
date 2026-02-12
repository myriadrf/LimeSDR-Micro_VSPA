/* SPDX-License-Identifier: BSD-3-Clause */

/*
 * Copyright 2024 NXP
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <vspa/intrinsics.h>

#include "l1-trace.h"
#include "ippu.h"
#include "vcpu.h"

#if L1_TRACE

l1_trace_data_t l1_trace_data[L1_TRACE_SIZE] __attribute__((aligned(64)));
uint32_t l1_trace_index;
volatile uint32_t l1_trace_disable;

void l1_trace_clear(void) {
    for (l1_trace_index = 0; l1_trace_index < L1_TRACE_SIZE; l1_trace_index++) {
        l1_trace_data[l1_trace_index].cnt = 0;
        l1_trace_data[l1_trace_index].msg = 0;
        l1_trace_data[l1_trace_index].param = 0;
    }
    l1_trace_index = 0;
}

#pragma cplusplus on
void l1_trace(uint32_t msg) {
    if (l1_trace_disable)
        return;

    l1_trace_data[l1_trace_index].cnt = ccnt_read();
    l1_trace_data[l1_trace_index].msg = msg;
    l1_trace_data[l1_trace_index].param = 0;
    l1_trace_index++;

    if (l1_trace_index >= L1_TRACE_SIZE) {
        l1_trace_index = 0;
    }
}

void l1_trace(uint32_t msg, uint32_t param) {
    if (l1_trace_disable)
        return;

    l1_trace_data[l1_trace_index].cnt = ccnt_read();
    l1_trace_data[l1_trace_index].msg = msg;
    l1_trace_data[l1_trace_index].param = param;
    l1_trace_index++;

    if (l1_trace_index >= L1_TRACE_SIZE) {
        l1_trace_index = 0;
    }
}
// no repeat version
void l1_trace_nr(uint32_t msg) {
    static uint32_t prev_index = 0xFFF, new_index = 0xFFF;

    if (l1_trace_disable)
        return;

    if ((new_index == l1_trace_index) && (msg == l1_trace_data[prev_index].msg)) {
        // keep track of how many times msg is sent consecutively
        l1_trace_data[prev_index].param = l1_trace_data[prev_index].param + 1;
        return;
    } else {

        prev_index = l1_trace_index;

        l1_trace_data[l1_trace_index].cnt = ccnt_read();
        l1_trace_data[l1_trace_index].msg = msg;
        l1_trace_data[l1_trace_index].param = 0;
        l1_trace_index++;

        if (l1_trace_index >= L1_TRACE_SIZE) {
            l1_trace_index = 0;
        }

        new_index = l1_trace_index;
    }
}
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

        if (l1_trace_index >= L1_TRACE_SIZE) {
            l1_trace_index = 0;
        }

        new_index = l1_trace_index;
    }
}

#pragma cplusplus reset
#endif
