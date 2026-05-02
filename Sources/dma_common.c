// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#include "dma_common.h"

#include "dmac.h"

uint32_t dma_chan_mask(uint32_t dma_channel, uint8_t nb_dma) {
    uint32_t mask = 0;
    for (uint8_t i = 0; i < nb_dma; i++) {
#pragma loop_count(1, 16, 2, 0)
        mask |= 1 << (dma_channel + i);
    }
    return mask;
}

uint32_t xfers_to_process(uint32_t dma_mask, const struct MemoryFIFO *fifo) {
    const uint32_t dma_pending = dmac_is_enabled(dma_mask);
    const uint32_t dma_done = dmac_is_complete(dma_mask) == dma_mask;
    uint32_t xfers_done = 0;
    if (dma_done) {
        dmac_clear_complete(dma_mask);
        xfers_done = 1; // could be 2 if dma completed both DMA FIFO entries
    }
    if (!dma_pending)
        xfers_done = fifo_size(fifo); // no pending transfers, consider all of them done
    return xfers_done;
}