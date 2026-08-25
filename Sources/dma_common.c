// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#include "dma_common.h"

#include "dmac.h"
#include "l1-trace.h"

void wait_for_dma(uint32_t dma_mask) {
    do { // wait
    } while (dmac_is_complete(dma_mask) != dma_mask && dmac_is_running(dma_mask));
    dmac_clear_complete(dma_mask);
}
