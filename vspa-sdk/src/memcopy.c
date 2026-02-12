// SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
// Copyright 2020 - 2025 the original authors

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <vspa/intrinsics.h>

#include "vcpu.h"

void memcpy32_w2w(void *d, void *s, size_t n) {
    uint32_t tmp1, tmp2, tmp3, tmp4;
    uint32_t *src_addr, *dst_addr;
    int loop_count;

    loop_count = n >> 2;

    src_addr = (uint32_t *)s;
    dst_addr = (uint32_t *)d;

    for (int count_word = 0; count_word < loop_count; count_word++) {
#pragma loop_count(0, 4096)
        tmp1 = *src_addr;
        src_addr++;
        tmp2 = *src_addr;
        src_addr++;
        tmp3 = *src_addr;
        src_addr++;
        tmp4 = *src_addr;
        src_addr++;
        *dst_addr = tmp1;
        dst_addr++;
        *dst_addr = tmp2;
        dst_addr++;
        *dst_addr = tmp3;
        dst_addr++;
        *dst_addr = tmp4;
        dst_addr++;
    }

    /* number of words not multiple of 4 */
    if (n & 0x03) {
        tmp3 = src_addr[2];
        tmp2 = src_addr[1];
        tmp1 = src_addr[0];
        switch (n & 0x03) {
        case 3:
            dst_addr[2] = tmp3;
        case 2:
            dst_addr[1] = tmp2;
        case 1:
            dst_addr[0] = tmp1;
        }
    }
}
