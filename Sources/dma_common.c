// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#include "dma_common.h"

#include "dmac.h"
#include "l1-trace.h"

uint32_t dma_chan_mask(uint32_t dma_channel, uint8_t nb_dma) {
    uint32_t mask = 0;
    for (uint8_t i = 0; i < nb_dma; i++) {
#pragma loop_count(1, 16, 2, 0)
        mask |= 1 << (dma_channel + i);
    }
    return mask;
}

void wait_for_dma(uint32_t dma_mask) {
    do { // wait
    } while (dmac_is_complete(dma_mask) != dma_mask && dmac_is_running(dma_mask));
    dmac_clear_complete(dma_mask);
}
