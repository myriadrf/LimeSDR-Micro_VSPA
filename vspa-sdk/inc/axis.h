// SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
// Copyright 2018 - 2025   NXP Semiconductors

// =============================================================================
//! @file       axis.h
//! @brief      AXI slave interface.
//! @author     NXP Semiconductors.
// =============================================================================

#ifndef __AXIS_H__
#define __AXIS_H__

// -----------------------------------------------------------------------------
// Subscriber interface (64-bit):
// -----------------------------------------------------------------------------

#pragma cplusplus on
static inline void axis_disable(void) {
    iowr(AXISLV_GOEN0, 0);
    iowr(AXISLV_GOEN1, 0);
}

static inline void axis_disable(uint64_t mask) {
    iowr(AXISLV_GOEN0, LOWORD(mask), 0);
    iowr(AXISLV_GOEN1, HIWORD(mask), 0);
}
#pragma cplusplus reset

#pragma cplusplus on
static inline void axis_enable(void) {
    iowr(AXISLV_GOEN0, 0xFFFFFFFF);
    iowr(AXISLV_GOEN1, 0xFFFFFFFF);
}

static inline void axis_enable(uint64_t mask) {
    iowr(AXISLV_GOEN0, LOWORD(mask), LOWORD(mask));
    iowr(AXISLV_GOEN1, HIWORD(mask), HIWORD(mask));
}
#pragma cplusplus reset

#pragma cplusplus on
static inline uint64_t axis_is_pending(void) { return MAKEDWORD(iord(AXISLV_FLAGS1), iord(AXISLV_FLAGS0)); }

static inline uint64_t axis_is_pending(uint64_t mask) {
    return MAKEDWORD(iord(AXISLV_FLAGS1, HIWORD(mask)), iord(AXISLV_FLAGS0, LOWORD(mask)));
}
#pragma cplusplus reset

#pragma cplusplus on
static inline void axis_clear(void) {
    iowr(AXISLV_FLAGS0, 0xFFFFFFFF);
    iowr(AXISLV_FLAGS1, 0xFFFFFFFF);
}

static inline void axis_clear(uint64_t mask) {
    iowr(AXISLV_FLAGS0, LOWORD(mask), LOWORD(mask));
    iowr(AXISLV_FLAGS1, HIWORD(mask), HIWORD(mask));
}
#pragma cplusplus reset

#pragma cplusplus on
static inline uint64_t axis_is_enabled(void) { return MAKEDWORD(iord(AXISLV_GOEN1), iord(AXISLV_GOEN0)); }

static inline uint64_t axis_is_enabled(uint64_t mask) {
    return MAKEDWORD(iord(AXISLV_GOEN1, HIWORD(mask)), iord(AXISLV_GOEN0, LOWORD(mask)));
}
#pragma cplusplus reset

// -----------------------------------------------------------------------------
// Client interface (32-bit):
// -----------------------------------------------------------------------------
#pragma cplusplus on
static inline void axis_lo_disable(void) { iowr(AXISLV_GOEN0, 0); }

static inline void axis_lo_disable(uint32_t mask) { iowr(AXISLV_GOEN0, mask, 0); }
#pragma cplusplus reset

#pragma cplusplus on
static inline void axis_hi_disable(void) { iowr(AXISLV_GOEN1, 0); }

static inline void axis_hi_disable(uint32_t mask) { iowr(AXISLV_GOEN1, mask, 0); }
#pragma cplusplus reset

#pragma cplusplus on
static inline void axis_lo_enable(void) { iowr(AXISLV_GOEN0, 0xFFFFFFFF); }

static inline void axis_lo_enable(uint32_t mask) { iowr(AXISLV_GOEN0, mask, mask); }
#pragma cplusplus reset

#pragma cplusplus on
static inline void axis_hi_enable(void) { iowr(AXISLV_GOEN1, 0xFFFFFFFF); }

static inline void axis_hi_enable(uint32_t mask) { iowr(AXISLV_GOEN1, mask, mask); }
#pragma cplusplus reset

#pragma cplusplus on
static inline uint32_t axis_lo_is_pending(void) { return iord(AXISLV_FLAGS0); }

static inline uint32_t axis_lo_is_pending(uint32_t mask) { return iord(AXISLV_FLAGS0, mask); }
#pragma cplusplus reset

#pragma cplusplus on
static inline uint32_t axis_lo_event(void) { return iord(AXISLV_FLAGS0) & iord(AXISLV_GOEN0); }

static inline uint32_t axis_lo_event(uint32_t mask) { return iord(AXISLV_FLAGS0, mask) & iord(AXISLV_GOEN0, mask); }
#pragma cplusplus reset

#pragma cplusplus on
static inline uint32_t axis_hi_is_pending(void) { return iord(AXISLV_FLAGS1); }

static inline uint32_t axis_hi_is_pending(uint32_t mask) { return iord(AXISLV_FLAGS1, mask); }
#pragma cplusplus reset

#pragma cplusplus on
static inline uint32_t axis_hi_event(void) { return iord(AXISLV_FLAGS1) & iord(AXISLV_GOEN1); }

static inline uint32_t axis_hi_event(uint32_t mask) { return iord(AXISLV_FLAGS1, mask) & iord(AXISLV_GOEN1, mask); }
#pragma cplusplus reset

#pragma cplusplus on
static inline void axis_lo_clear(void) { iowr(AXISLV_FLAGS0, 0xFFFFFFFF); }

static inline void axis_lo_clear(uint32_t mask) { iowr(AXISLV_FLAGS0, mask, mask); }
#pragma cplusplus reset

#pragma cplusplus on
static inline void axis_hi_clear(void) { iowr(AXISLV_FLAGS1, 0xFFFFFFFF); }

static inline void axis_hi_clear(uint32_t mask) { iowr(AXISLV_FLAGS1, mask, mask); }
#pragma cplusplus reset

#pragma cplusplus on
static inline uint32_t axis_lo_is_enabled(void) { return iord(AXISLV_GOEN0); }

static inline uint32_t axis_lo_is_enabled(uint32_t mask) { return iord(AXISLV_GOEN0, mask); }
#pragma cplusplus reset

#pragma cplusplus on
static inline uint32_t axis_hi_is_enabled(void) { return iord(AXISLV_GOEN1); }

static inline uint32_t axis_hi_is_enabled(uint32_t mask) { return iord(AXISLV_GOEN1, mask); }
#pragma cplusplus reset

// -----------------------------------------------------------------------------
// Publisher interface:
// -----------------------------------------------------------------------------

//! @brief      AXI slave flag bit mask data type.
//!
//! This data type represents the AXI slave flags, to be transferred to the AXI
//! slave flags of another VSPA core.
struct axis_flag_mask_t {
    uint32_t lsb __align_axi; //!< 32 least significant bits.
    uint32_t msb;             //!< 32 most significant bits.
};

//! @brief      AXI slave flag bit mask.
//!
//! This structure stores the AXI slave flags, to be transferred to the AXI
//! slave flags of another VSPA core.
extern struct axis_flag_mask_t axis_flag_mask __align_axi;

//! @brief      AXI slave flag address table.
//!
//! This array stores the address of the AXI slave flags of all VSPA cores.
extern uint32_t const axis_flag_addr[VSPA_ATID_CNT];

#pragma cplusplus on

//! @brief      Signal an AXI slave event on a target VSPA core.
//! @param      chan    specifies the DMA channel to use to write the flags.
//! @param      atid    specifies the target VSPA core.
//! @param      mask    specifies the flags to set.
//! @return     This function does not return a value.
//!
//! This function writes the AXI slave flags specified in @a mask to the target
//! VSPA core @atid, using the DMA channel @a chan, to signal an AXI slave
//! event.
static inline void axis_signal(uint32_t chan, vspa_atid_t atid, uint64_t mask) {
    // Store flag mask in data memory:
    axis_flag_mask.lsb = LOWORD(mask);
    axis_flag_mask.msb = HIWORD(mask);

    // Transfer flag mask:
    dmac_enable(DMAC_WR | chan, 8, BASE_VSPA_FLAG(atid), &axis_flag_mask);
}

//! @brief      Signal an AXI slave event on a target VSPA core.
//! @param      chan    specifies the DMA channel to use to write the flags.
//! @param      atid    specifies the target VSPA core.
//! @param      mask    specifies the address of the flags to set.
//! @return     This function does not return a value.
//!
//! This function writes the AXI slave flags specified in @a mask to the target
//! VSPA core @atid, using the DMA channel @a chan, to signal an AXI slave
//! event.
static inline void axis_signal(uint32_t chan, vspa_atid_t atid, struct axis_flag_mask_t *mask) {
    // Transfer flag mask:
    dmac_enable(DMAC_WR | chan, 8, BASE_VSPA_FLAG(atid), mask);
}

//! @brief      Signal an AXI slave event on a target VSPA core.
//! @param      chan    specifies the DMA channel to use to write the flags.
//! @param      atid    specifies the target VSPA core.
//! @return     This function does not return a value.
//! @pre        The AXI slave flags @c axis_flag_mask are set.
//!
//! This function writes the AXI slave flags previously stored
//! in @c axis_flag_mask to the target VSPA core @atid,
//! using the DMA channel @a chan, to signal an AXI slave event.
static inline void axis_signal(uint32_t chan, vspa_atid_t atid) {
    // Transfer flag mask:
    dmac_enable(DMAC_WR | chan, 8, BASE_VSPA_FLAG(atid), &axis_flag_mask);
}

#pragma cplusplus reset

//! @brief      Signal an AXI slave event on a target VSPA core.
//! @param      chan    specifies the DMA channel to use to write the flags.
//! @param      atid    specifies the target VSPA core.
//! @param      mask    specifies the flags to set.
//! @return     This function does not return a value.
//!
//! This function writes the AXI slave flags specified in @a mask to the target
//! VSPA core @atid, using the DMA channel @a chan, to signal an AXI slave
//! event.
static inline void axis_lo_signal(uint32_t chan, vspa_atid_t atid, uint32_t mask) {
    // Store flag mask in data memory:
    axis_flag_mask.lsb = mask;
    axis_flag_mask.msb = 0;

    // Transfer flag mask:
    dmac_enable(DMAC_WR | chan, 8, BASE_VSPA_FLAG(atid), &axis_flag_mask);
}

//! @brief      Signal an AXI slave event on a target VSPA core.
//! @param      chan    specifies the DMA channel to use to write the flags.
//! @param      atid    specifies the target VSPA core.
//! @param      mask    specifies the flags to set.
//! @return     This function does not return a value.
//!
//! This function writes the AXI slave flags specified in @a mask to the target
//! VSPA core @atid, using the DMA channel @a chan, to signal an AXI slave
//! event.
static inline void axis_hi_signal(uint32_t chan, vspa_atid_t atid, uint32_t mask) {
    // Store flag mask in data memory:
    axis_flag_mask.lsb = 0;
    axis_flag_mask.msb = mask;

    // Transfer flag mask:
    dmac_enable(DMAC_WR | chan, 8, BASE_VSPA_FLAG(atid), &axis_flag_mask);
}

//! @brief      Preloads common axis_flag_mask instance and dma registers
//! @param      hi    32 most significant bits
//! @param      lo    32 least significant bits
//! @return     This function does not return a value
//!
//! Efficient way to sent same axis flag to several cores.
static inline void axis_dma_preload(uint32_t hi, uint32_t lo) {
    axis_flag_mask.lsb = lo;
    axis_flag_mask.msb = hi;
    iowr(DMA_DMEM_PRAM_ADDR, (uint32_t)&axis_flag_mask);
    iowr(DMA_AXI_BYTE_CNT, 8);
}

//! @brief      Signal preloaded AXI slave flag on a target VSPA core
//! @param      chan    specifies the DMA channel to use to write the flags
//! @param      atid    specifies the target VSPA core
//!
//! Efficient way to sent same axis flag to several cores.
static inline void axis_dma_enable(uint32_t chan, vspa_atid_t atid) {
    iowr(DMA_AXI_ADDRESS, (uint32_t)BASE_VSPA_FLAG(atid));
    iowr(DMA_XFR_CTRL, DMAC_WR | chan);
}

#endif // __AXIS_H__
