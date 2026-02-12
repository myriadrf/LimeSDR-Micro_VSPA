// SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
// Copyright 2019 - 2025   NXP Semiconductors

// =============================================================================
//! @file       pdma.h
//! @brief      Program memory direct memory access controller physical driver.
//! @author     NXP Semiconductors.
// =============================================================================

#ifndef __PDMA_H__
#define __PDMA_H__

#if defined(__LA1224__)

#define PDMA_TRIG_VCPU (0b1 << 0)

static inline void pdma_enable(uint32_t ctrl, size_t size, uint32_t lma, uint32_t rma) {
    iowr(AD_PRAM_ADDR, rma);
    iowr(AD_AXI_ADDR, lma);
    iowr(AD_BYTE_COUNT, size);
    iowr(AD_CTRL, ctrl);
}

static inline uint32_t pdma_is_complete(void) { return iord(AD_STATUS, 0b1 << 0); }

static inline uint32_t pdma_err_cfg(void) { return iord(AD_STATUS, 0b1 << 1); }

static inline uint32_t pdma_err_axi(void) { return iord(AD_STATUS, 0b1 << 2); }

static inline uint32_t pdma_event(void) { return iord(AD_STATUS, 0b1 << 3); }

static inline uint32_t pdma_is_busy(void) { return iord(AD_STATUS, 0b1 << 4); }

static inline void pdma_clear(void) { iowr(AD_STATUS, 0xF, 0xF); }

#endif // defined(__LA1224__)

#endif // PDMA_H__
