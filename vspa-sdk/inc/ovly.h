// SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
// Copyright 2018 - 2025   NXP Semiconductors

// =============================================================================
//! @file       ovly.h
//! @brief      Overlay management interface.
//! @author     NXP Semiconductors.
// =============================================================================

#ifndef __OVLY_H__
#define __OVLY_H__

#include "pdma.h"

// Debugger interface:

//! @brief      Overlay descriptor.
struct ovly_t {
    uint32_t lma; //!< Overlay load byte address.
    uint32_t rma; //!< Overlay run byte address.
    size_t size;  //!< Overlay byte size.
};

//! @brief      Array of overlay descriptors.
extern struct ovly_t ovly_table[];

#define OVLY_DATA_SIZE(name) __ovly_data_size_##name
#define OVLY_DATA_LMA(name) __ovly_data_lma_##name
#define OVLY_DATA_RMA(name) __ovly_data_rma_##name

#define OVLY_BSS_SIZE(name) __ovly_bss_size_##name
#define OVLY_BSS_LMA(name) __ovly_bss_lma_##name
#define OVLY_BSS_RMA(name) __ovly_bss_rma_##name

#define OVLY_BSS_NC_SIZE(name) __ovly_bss_nc_size_##name
#define OVLY_BSS_NC_LMA(name) __ovly_bss_nc_lma_##name
#define OVLY_BSS_NC_RMA(name) __ovly_bss_nc_rma_##name

#define OVLY_CODE_SIZE(name) __ovly_code_size_##name
#define OVLY_CODE_LMA(name) __ovly_code_lma_##name
#define OVLY_CODE_RMA(name) __ovly_code_rma_##name

#define OVLY_DATA(name)                    \
    extern uint32_t OVLY_DATA_LMA(name)[]; \
    extern uint32_t OVLY_DATA_RMA(name)[]; \
    extern uint32_t OVLY_DATA_SIZE(name)[]

#define OVLY_BSS(name)                    \
    extern uint32_t OVLY_BSS_LMA(name)[]; \
    extern uint32_t OVLY_BSS_RMA(name)[]; \
    extern uint32_t OVLY_BSS_SIZE(name)[]

#define OVLY_CODE(name)                    \
    extern uint32_t OVLY_CODE_LMA(name)[]; \
    extern uint32_t OVLY_CODE_RMA(name)[]; \
    extern uint32_t OVLY_CODE_SIZE(name)[]

//! @brief      Overlay enable bit map.
extern uint32_t ovly_mapped;

//! @brief      Signal debugger an update to the overlay enable bit map.
extern void ovly_debug_event(void);

//! @brief      Load a VCPU program image.
//! @return     This function does not return a value.
static inline void ovly_load_text(uint32_t index, uint32_t chan) {
    uint32_t lma = ovly_table[index].lma; // Load address in AXI space.
    uint32_t rma = ovly_table[index].rma; // Run address in VCPU PMEM space.
    size_t size = ovly_table[index].size;
    dmac_enable(DMAC_VPRAM | chan, size, lma, rma);
}

static inline void ovly_load_data(uint32_t index, uint32_t chan) {
    size_t size = ovly_table[index].size;
    if (size > 0) {
        uint32_t lma = ovly_table[index].lma; // Load address in AXI space.
        uint32_t rma = ovly_table[index].rma; // Run address in VCPU PMEM space.
        dmac_enable(DMAC_RD | chan, size, lma, rma);
    }
}

#if defined(__LA1224__)
static inline void ovly_load_code(uint32_t index, uint32_t vcpu_go_en)
#else
static inline void ovly_load_code(uint32_t index, uint32_t chan)
#endif
{
    size_t size = ovly_table[index].size;
    if (size > 0) {
        uint32_t lma = ovly_table[index].lma; // Load address in AXI space.
        uint32_t rma = ovly_table[index].rma; // Run address in VCPU PMEM space.
#if defined(__LA1224__)
        pdma_enable(vcpu_go_en, size, lma, rma);
#else
        dmac_enable(DMAC_VPRAM | chan, size, lma, rma);
#endif
    }
}

static inline void ovly_clear_bss(uint32_t index) {
    size_t size = ovly_table[index].size >> 1;
    if (size > 0) {
        memclrvec((void *)(ovly_table[index].rma), DIV_CEIL((size_t)(size), SIZEVEC16));
    }
}

static inline void ovly_set(uint32_t mask) { ovly_mapped = mask; }

static inline void ovly_clear(void) { ovly_mapped = 0; }

//! @brief      Map overlay(s) in the overlay enable bit map.
//! @param      mask    Bit mask specifying the overlays.
//! @return     This function does not return a value.
static inline void ovly_map(uint32_t mask) { ovly_mapped |= mask; }

//! @brief      Unmap overlay(s) in the overlay enable bit map.
//! @param      mask    Bit mask specifying the overlays.
//! @return     This function does not return a value.
static inline void ovly_unmap(uint32_t mask) { ovly_mapped &= ~mask; }

//! @brief      Query if one or more overlays are already loaded.
//! @param      mask        Bit mask specifying the overlay(s) to test.
//! @retval     zero        Specified overlays are not mapped.
//! @retval     non-zero    At least 1 specified overlay is mapped.
static inline uint32_t ovly_is_mapped(uint32_t mask) { return (ovly_mapped & mask); }

#endif // __OVLY_H__
