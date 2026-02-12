// SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
// Copyright 20182023 - 2025   NXP Semiconductors

// =============================================================================
//! @file       dmac.h
//! @brief      Direct memory access controller interface.
//! @author     NXP Semiconductors.
// =============================================================================

#ifndef __DMAC_H__
#define __DMAC_H__

// -----------------------------------------------------------------------------
// Control interface:
// -----------------------------------------------------------------------------

// Trigger options:
#define DMAC_TRIG_IPPU (0b1 << 12) //!< Trigger IPPU on transfer completion.
#define DMAC_TRIG_VCPU (0b1 << 13) //!< Trigger VCPU on transfer completion.
#define DMAC_TRIG_IRQ (0b1 << 14)  //!< Trigger IRQ  on transfer completion.
#define DMAC_TRIG_FECU (0b1 << 22) //!< Trigger FECU on transfer completion.

// Pending options:
#define DMAC_PEND_EXT (0b1 << 16)  //!< Transfer pending on external trigger.
#define DMAC_PEND_IPPU (0b1 << 17) //!< Transfer pending on IPPU procedure completion.
#define DMAC_PEND_FECU (0b1 << 23) //!< Transfer pending on FECU operation completion.

// Pointer reset request:
#define DMAC_PRST_REQ (0b1 << 15)

// FIFO mode:
#define DMAC_FIFO_HS ((0b1 << 11))
#define DMAC_FIFO ((0b1 << 11) | DMAC_PEND_EXT)
#define DMAC_FIFO_RESET ((0b1 << 11) | DMAC_PRST_REQ)

// Program memory access:
#define DMAC_VPRAM (0b010 << 8) //!< Transfer to VCPU program memory.
#define DMAC_IPRAM (0b011 << 8) //!< Transfer to IPPU program memory.

// Turbo read:
#define DMAC_MBRE (0b1 << 7) //!< Enable multi-burst read.

// Transfer modes:
#define DMAC_RD (0b000 << 8)                         //!< Read from AXI
#define DMAC_RDC (0b001 << 8)                        //!< Read from AXI and convert to sign-magnitude.
#define DMAC_WR (0b110 << 8)                         //!< Write to AXI.
#define DMAC_WRC (0b111 << 8)                        //!< Write to AXI and convert to 2's complement.
#define DMAC_DI8 ((0b100 << 8) | (0b001 << 18))      //!<  8-bit write to AXI.
#define DMAC_DI4_MSB0 ((0b100 << 8) | (0b010 << 18)) //!<  4-bit write to AXI.
#define DMAC_DI4_MSB1 ((0b100 << 8) | (0b011 << 18)) //!<  4-bit write to AXI.
#define DMAC_DI16 ((0b100 << 8) | (0b100 << 18))     //!< 16-bit write to AXI.
#define DMAC_DI32 ((0b100 << 8) | (0b101 << 18))     //!< 32-bit write to AXI.

// Sideband signaling:
#define DMAC_SIDEBAND(x) ((x) << 24)

#pragma cplusplus on
static inline void dmac_enable(uint32_t ctrl, size_t size, void const *addr, void const *data) {
    iowr(DMA_DMEM_PRAM_ADDR, (uint32_t)data);
    iowr(DMA_AXI_ADDRESS, (uint32_t)addr);
    iowr(DMA_AXI_BYTE_CNT, size);
    iowr(DMA_XFR_CTRL, ctrl);
}

static inline void dmac_enable(uint32_t ctrl, size_t size, uint32_t addr, void const *data) {
    iowr(DMA_DMEM_PRAM_ADDR, (uint32_t)data);
    iowr(DMA_AXI_ADDRESS, (uint32_t)addr);
    iowr(DMA_AXI_BYTE_CNT, size);
    iowr(DMA_XFR_CTRL, ctrl);
}

static inline void dmac_enable(uint32_t ctrl, size_t size, uint32_t addr, uint32_t data) {
    iowr(DMA_DMEM_PRAM_ADDR, data);
    iowr(DMA_AXI_ADDRESS, addr);
    iowr(DMA_AXI_BYTE_CNT, size);
    iowr(DMA_XFR_CTRL, ctrl);
}

//! @brief  Reduced instruction dma commands:
//!         only indicated register instruction will be executed
//!         omitted registers assumed to be same
//!         v = vspa address
//!         a = axi space address
//!         s = size
//!         c = ctrl word
//! --------------------------------------------------------------
//!         v = vspa address
//!         a = axi space address
//!         c = ctrl word
static inline void dmac_enable_v_a_c(uint32_t ctrl, uint32_t addr, void const *data) {
    iowr(DMA_DMEM_PRAM_ADDR, (uint32_t)data);
    iowr(DMA_AXI_ADDRESS, addr);
    iowr(DMA_XFR_CTRL, ctrl);
}
static inline void dmac_enable_v_a_c(uint32_t ctrl, uint32_t addr, uint32_t data) {
    iowr(DMA_DMEM_PRAM_ADDR, (uint32_t)data);
    iowr(DMA_AXI_ADDRESS, addr);
    iowr(DMA_XFR_CTRL, ctrl);
}
static inline void dmac_enable_v_c(uint32_t ctrl, uint32_t data) {
    iowr(DMA_DMEM_PRAM_ADDR, (uint32_t)data);
    iowr(DMA_XFR_CTRL, ctrl);
}
static inline void dmac_enable_v_c(uint32_t ctrl, void const *data) {
    iowr(DMA_DMEM_PRAM_ADDR, (uint32_t)data);
    iowr(DMA_XFR_CTRL, ctrl);
}
//!         a = axi space address
//!         c = ctrl word
static inline void dmac_enable_a_c(uint32_t ctrl, uint32_t addr) {
    iowr(DMA_AXI_ADDRESS, addr);
    iowr(DMA_XFR_CTRL, ctrl);
}
static inline void dmac_enable_a_c(uint32_t ctrl, void const *addr) {
    iowr(DMA_AXI_ADDRESS, (uint32_t)addr);
    iowr(DMA_XFR_CTRL, ctrl);
}
//! function same as above...called 'prep' instead of 'enable'
//! since it does not write the control word, therefore it does not launch
//! a dma, but does write to some of the 4 dma registers
//!         v = vspa address
//!         s = size
static inline void dmac_prep_v_s(size_t size, uint32_t data) {
    iowr(DMA_DMEM_PRAM_ADDR, data);
    iowr(DMA_AXI_BYTE_CNT, size);
}
static inline void dmac_prep_v_s(size_t size, void const *data) {
    iowr(DMA_DMEM_PRAM_ADDR, (uint32_t)data);
    iowr(DMA_AXI_BYTE_CNT, size);
}
static inline void dmac_prep_a_s(size_t size, uint32_t addr) {
    iowr(DMA_AXI_ADDRESS, addr);
    iowr(DMA_AXI_BYTE_CNT, size);
}
static inline void dmac_prep_a_s(size_t size, void const *addr) {
    iowr(DMA_AXI_ADDRESS, (uint32_t)addr);
    iowr(DMA_AXI_BYTE_CNT, size);
}
static inline void dmac_prep_s(size_t size) { iowr(DMA_AXI_BYTE_CNT, size); }
static inline void dmac_prep_v(uint32_t data) { iowr(DMA_DMEM_PRAM_ADDR, (uint32_t)data); }
static inline void dmac_prep_a(uint32_t addr) { iowr(DMA_AXI_ADDRESS, addr); }
#pragma cplusplus reset

#pragma cplusplus on

static inline void dmac_abort(void) { iowr(DMA_STAT_ABORT, 0xFFFFFFFF); }

static inline void dmac_abort(uint32_t mask) { iowr(DMA_STAT_ABORT, mask, mask); }

#pragma cplusplus reset

#pragma cplusplus on

static inline void dmac_clear_complete(void) { iowr(DMA_COMP_STAT, 0xFFFFFFFF); }

static inline void dmac_clear_complete(uint32_t mask) { iowr(DMA_COMP_STAT, mask, mask); }

#pragma cplusplus reset

#pragma cplusplus on

static inline void dmac_clear_errxfr(void) { iowr(DMA_XFRERR_STAT, 0xFFFFFFFF); }

static inline void dmac_clear_errxfr(uint32_t mask) { iowr(DMA_XFRERR_STAT, mask, mask); }

#pragma cplusplus reset

#pragma cplusplus on

static inline void dmac_clear_errcfg(void) { iowr(DMA_CFGERR_STAT, 0xFFFFFFFF); }

static inline void dmac_clear_errcfg(uint32_t mask) { iowr(DMA_CFGERR_STAT, mask, mask); }

#pragma cplusplus reset

#pragma cplusplus on

static inline void dmac_clear_error(void) {
    dmac_clear_errxfr();
    dmac_clear_errcfg();
}

static inline void dmac_clear_error(uint32_t mask) {
    dmac_clear_errxfr(mask);
    dmac_clear_errcfg(mask);
}

#pragma cplusplus reset

#pragma cplusplus on

static inline void dmac_clear_event(void) { iowr(DMA_GO_STAT, 0xFFFFFFFF); }

static inline void dmac_clear_event(uint32_t mask) { iowr(DMA_GO_STAT, mask, mask); }

#pragma cplusplus reset

#pragma cplusplus on

static inline void dmac_clear(void) {
    dmac_clear_complete();
    dmac_clear_event();
}

static inline void dmac_clear(uint32_t mask) {
    dmac_clear_complete(mask);
    dmac_clear_event(mask);
}

static inline void dmac_reset(void) {
    dmac_abort();
    dmac_clear();
    dmac_clear_error();
}

static inline void dmac_reset(uint32_t mask) {
    dmac_abort(mask);
    dmac_clear(mask);
    dmac_clear_error(mask);
}

#pragma cplusplus reset

// -----------------------------------------------------------------------------
// Status interface:
// -----------------------------------------------------------------------------

#pragma cplusplus on

static inline uint32_t dmac_is_enabled(void) { return iord(DMA_STAT_ABORT); }

static inline uint32_t dmac_is_enabled(uint32_t mask) { return iord(DMA_STAT_ABORT, mask); }

#pragma cplusplus reset

#pragma cplusplus on

static inline uint32_t dmac_is_complete(void) { return iord(DMA_COMP_STAT); }

static inline uint32_t dmac_is_complete(uint32_t mask) { return iord(DMA_COMP_STAT, mask); }

#pragma cplusplus reset

#pragma cplusplus on

static inline uint32_t dmac_errxfr(void) { return iord(DMA_XFRERR_STAT); }

static inline uint32_t dmac_errxfr(uint32_t mask) { return iord(DMA_XFRERR_STAT, mask); }

#pragma cplusplus reset

#pragma cplusplus on

static inline uint32_t dmac_errcfg(void) { return iord(DMA_CFGERR_STAT); }

static inline uint32_t dmac_errcfg(uint32_t mask) { return iord(DMA_CFGERR_STAT, mask); }

#pragma cplusplus reset

#pragma cplusplus on

static inline uint32_t dmac_error(void) { return dmac_errxfr() | dmac_errcfg(); }

static inline uint32_t dmac_error(uint32_t mask) { return (dmac_errxfr(mask) | dmac_errcfg(mask)); }

#pragma cplusplus reset

#pragma cplusplus on

static inline uint32_t dmac_event(void) { return iord(DMA_GO_STAT); }

static inline uint32_t dmac_event(uint32_t mask) { return iord(DMA_GO_STAT, mask); }

#pragma cplusplus reset

#pragma cplusplus on

static inline uint32_t dmac_is_available(void) { return iord(DMA_FIFO_STAT); }

static inline uint32_t dmac_is_available(uint32_t mask) { return iord(DMA_FIFO_STAT, mask); }

static inline uint32_t dmac_is_running(uint32_t mask) { return iord(DMA_XRUN_STAT, mask); }

#pragma cplusplus reset

#endif // __DMAC_H__
