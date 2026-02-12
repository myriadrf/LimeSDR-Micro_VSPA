// SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
// Copyright 2017 - 2025   NXP Semiconductors

// =============================================================================
//! @file       iohw.h
//! @brief      Hardware I/O definitions.
//! @author     NXP Semiconductors.
// =============================================================================

#ifndef __HWIO_H__
#define __HWIO_H__

//! @ingroup    GROUP_VSPA_IOHW
//! @{

#define HWVERSION (0x000 >> 2)
#define SWVERSION (0x004 >> 2)
#define CONTROL (0x008 >> 2)
#define IRQEN (0x00C >> 2)
#define STATUS (0x010 >> 2)
#define VCPU_HOST_FLAGS0 (0x014 >> 2)
#define VCPU_HOST_FLAGS1 (0x018 >> 2)
#define HOST_VCPU_FLAGS0 (0x01C >> 2)
#define HOST_VCPU_FLAGS1 (0x020 >> 2)
#define EXT_GO_ENA (0x028 >> 2)
#define EXT_GO_STAT (0x02C >> 2)
#define ILLOP_STATUS (0x030 >> 2)
#define PARAM0 (0x040 >> 2)
#define PARAM1 (0x044 >> 2)
#define PARAM2 (0x048 >> 2)
#define VCPU_DMEM_BYTES (0x04C >> 2)
#define THREAD_CTRL_STAT (0x050 >> 2)
#define PROT_FAULT_STAT (0x054 >> 2)
#define EXCEPTION_CTRL (0x058 >> 2)
#define EXCEPTION_STAT (0x05C >> 2)
#define AXISLV_FLAGS0 (0x060 >> 2)
#define AXISLV_FLAGS1 (0x064 >> 2)
#define AXISLV_GOEN0 (0x068 >> 2)
#define AXISLV_GOEN1 (0x06C >> 2)
#define PLAT_INT_0 (0x070 >> 2)
#define PLAT_OUT_0 (0x080 >> 2)
#define CYC_CNTR_MSB (0x098 >> 2)
#define CYC_CNTR_LSB (0x09C >> 2)
#define DMA_DMEM_PRAM_ADDR (0x0B0 >> 2)
#define DMA_AXI_ADDRESS (0x0B4 >> 2)
#define DMA_AXI_BYTE_CNT (0x0B8 >> 2)
#define DMA_XFR_CTRL (0x0BC >> 2)
#define DMA_STAT_ABORT (0x0C0 >> 2)
#define DMA_IRQ_STAT (0x0C4 >> 2)
#define DMA_COMP_STAT (0x0C8 >> 2)
#define DMA_XFRERR_STAT (0x0CC >> 2)
#define DMA_CFGERR_STAT (0x0D0 >> 2)
#define DMA_XRUN_STAT (0x0D4 >> 2)
#define DMA_GO_STAT (0x0D8 >> 2)
#define DMA_FIFO_STAT (0x0DC >> 2)
#define AD_STATUS (0x0E0 >> 2)
#define LD_RF_CONTROL (0x100 >> 2)
#define LD_RF_TB_REAL_0 (0x104 >> 2)
#define LD_RF_TB_IMAG_0 (0x108 >> 2)
#define LD_RF_TB_REAL_1 (0x10C >> 2)
#define LD_RF_TB_IMAG_1 (0x110 >> 2)
#define LD_RF_TB_REAL_2 (0x114 >> 2)
#define LD_RF_TB_IMAG_2 (0x118 >> 2)
#define LD_RF_TB_REAL_3 (0x11C >> 2)
#define LD_RF_TB_IMAG_3 (0x120 >> 2)
#define LD_RF_TB_REAL_4 (0x124 >> 2)
#define LD_RF_TB_IMAG_4 (0x128 >> 2)
#define LD_RF_TB_REAL_5 (0x12C >> 2)
#define LD_RF_TB_IMAG_5 (0x130 >> 2)
#define LD_RF_TB_REAL_6 (0x134 >> 2)
#define LD_RF_TB_IMAG_6 (0x138 >> 2)
#define LD_RF_TB_REAL_7 (0x13C >> 2)
#define LD_RF_TB_IMAG_7 (0x140 >> 2)
#define VCPU_GO_ADDR (0x180 >> 2)
#define VCPU_GO_STACK (0x184 >> 2)
#define AD_PRAM_ADDR (0x188 >> 2)
#define AD_AXI_ADDR (0x18C >> 2)
#define AD_BYTE_COUNT (0x190 >> 2)
#define AD_CTRL (0x194 >> 2)
#define VCPU_MODE0 (0x400 >> 2)
#define VCPU_MODE1 (0x404 >> 2)
#define VCPU_CREG0 (0x408 >> 2)
#define VCPU_CREG1 (0x40C >> 2)
#ifndef ST_UL_VEC_LEN
#define ST_UL_VEC_LEN (0x410 >> 2)
#endif
#ifndef VP_CONTROL
#define VP_CONTROL (0x414 >> 2)
#endif
#define XC_CONTROL (0x418 >> 2)
#define GPIN0 (0x500 >> 2)
#define GPOUT0 (0x580 >> 2)
#define DQM_SMALL (0x600 >> 2)
#define DQM_LARGE_MSB (0x604 >> 2)
#define DQM_LARGE_LSB (0x608 >> 2)
#define VCPU_DBG_OUT_32 (0x620 >> 2)
#define VCPU_DBG_OUT_64_MSB (0x624 >> 2)
#define VCPU_DBG_OUT_64_LSB (0x628 >> 2)
#define VCPU_DBG_IN_32 (0x62C >> 2)
#define VCPU_DBG_IN_64_MSB (0x630 >> 2)
#define VCPU_DBG_IN_64_LSB (0x634 >> 2)
#define VCPU_DBG_MBOX_STATUS (0x638 >> 2)
#define VCPU_OUT_0_MSB (0x640 >> 2)
#define VCPU_OUT_0_LSB (0x644 >> 2)
#define VCPU_OUT_1_MSB (0x648 >> 2)
#define VCPU_OUT_1_LSB (0x64C >> 2)
#define VCPU_IN_0_MSB (0x650 >> 2)
#define VCPU_IN_0_LSB (0x654 >> 2)
#define VCPU_IN_1_MSB (0x658 >> 2)
#define VCPU_IN_1_LSB (0x65C >> 2)
#define VCPU_MBOX_STATUS (0x660 >> 2)
#define HOST_OUT_0_MSB (0x680 >> 2)
#define HOST_OUT_0_LSB (0x684 >> 2)
#define HOST_OUT_1_MSB (0x688 >> 2)
#define HOST_OUT_1_LSB (0x68C >> 2)
#define HOST_IN_0_MSB (0x690 >> 2)
#define HOST_IN_0_LSB (0x694 >> 2)
#define HOST_IN_1_MSB (0x698 >> 2)
#define HOST_IN_1_LSB (0x69C >> 2)
#define HOST_MBOX_STATUS (0x6A0 >> 2)
#define IPPUCONTROL (0x700 >> 2)
#define IPPUSTATUS (0x704 >> 2)
#define IPPURC (0x708 >> 2)
#define IPPUARGBASEADDR (0x70C >> 2)
#define IPPUHWVER (0x710 >> 2)
#define IPPUSWVER (0x714 >> 2)

#if !defined(__ASSEMBLER__)
// -----------------------------------------------------------------------------
// Data types:
// -----------------------------------------------------------------------------

//! @brief      I/O register designator data type.
//!
//! This data type describes the space of all I/O addresses,
//! that is the IP bus address space.
typedef uint32_t ioreg_t;

//! @brief      I/O register index data type.
//!
//! This data type describes an integer type that can be used to index into
//! an array of I/O addresses, like GPIO addresses for example.
typedef uint32_t ioindex_t;

// -----------------------------------------------------------------------------
// 32-bit access functions:
// -----------------------------------------------------------------------------

#pragma cplusplus on

//! @brief      Read a 32-bit I/O register.
//!
//! @param      reg     Specifies the I/O register address.
//! @return     The value read from the 32-bit register.
//!
//! This function returns the 32-bit value of the I/O register @a reg.
static inline uint32_t iord(ioreg_t reg) { return __ip_read(reg); }

//! @brief      Read a 32-bit I/O register with a mask.
//!
//! @param      reg     Specifies the I/O register address.
//! @param      mask    Specifies the bit mask to read with. Only bits set to 1
//!                     in the mask are read, others are returned as zero.
//! @return     The masked value read from the 32-bit register.
//!
//! This function returns the 32-bit value of the I/O register @a reg, masked
//! with the bit mask @a mask.
static inline uint32_t iord(ioreg_t reg, uint32_t mask) { return __ip_read(reg, mask); }

//! @brief      Write a 32-bit I/O register.
//!
//! @param      reg     Specifies the I/O register address.
//! @param      data    The value to write to the register.
//! @return     This function does not return a value.
//!
//! This function writes the value @a data to the 32-bit I/O register @a reg.
static inline void iowr(ioreg_t reg, uint32_t data) { __ip_write(reg, data); }

//! @brief      Write a 32-bit I/O register with a mask.
//!
//! @param      reg     Specifies the I/O register address.
//! @param      mask    Specifies the bit mask to write with. Only bits set to 1
//!                     in the mask are written, others are left untouched.
//! @param      data    The value to write to the register.
//! @return     This function does not return a value.
//!
//! This function writes the bits specified by the mask @a mask from the value
//! @a data to the 32-bit I/O register @a reg.
static inline void iowr(ioreg_t reg, uint32_t mask, uint32_t data) { __ip_write(reg, mask, data); }

#pragma cplusplus reset

// -----------------------------------------------------------------------------
// 64-bit access functions:
// -----------------------------------------------------------------------------

#pragma cplusplus on

//! @brief      Read a 64-bit I/O register.
//!
//! @param      reg     Specifies the I/O register address.
//! @return     The value read from the 64-bit register.
//!
//! This function returns the 64-bit value of the I/O register @a reg.
//!
//! @attention  reg is the address of the MSB register.
//! @note       MSBs are read first.
static inline uint64_t iord64(ioreg_t reg) {
    uint32_t msb = iord(reg + 0);
    uint32_t lsb = iord(reg + 1);
    return (((uint64_t)msb) << 32) | lsb;
}

//! @brief      Read a 64-bit I/O register with a mask.
//!
//! @param      reg     Specifies the I/O register address.
//! @param      mask    Specifies the bit mask to read with. Only bits set to 1
//!                     in the mask are read, others are returned as zero.
//! @return     The masked value read from the 64-bit register.
//!
//! This function returns the 64-bit value of the I/O register @a reg, masked
//! with the bit mask @a mask.
//!
//! @attention  reg is the address of the MSB register.
//! @note       MSBs are read first.
static inline uint32_t iord64(ioreg_t reg, uint64_t mask) {
    uint32_t msb = iord(reg + 0, mask >> 32);
    uint32_t lsb = iord(reg + 1, mask & 0x00000000FFFFFFFFLL);
    return (((uint64_t)msb) << 32) | lsb;
}

//! @brief      Write a 64-bit I/O register.
//!
//! @param      reg     Specifies the I/O register address.
//! @param      data    The value to write to the register.
//! @return     This function does not return a value.
//!
//! This function writes the value @a data to the 64-bit I/O register @a reg.
//!
//! @attention  reg is the address of the MSB register.
//! @note       MSBs are written first.
static inline void iowr64(ioreg_t reg, uint64_t data) {
    __ip_write(reg + 0, data >> 32);
    __ip_write(reg + 1, data & 0x00000000FFFFFFFFLL);
}

//! @brief      Write a 64-bit I/O register with a mask.
//!
//! @param      reg     Specifies the I/O register address.
//! @param      mask    Specifies the bit mask to write with. Only bits set to 1
//!                     in the mask are written, others are left untouched.
//! @param      data    The value to write to the register.
//! @return     This function does not return a value.
//!
//! This function writes the bits specified by the mask @a mask from the value
//! @a data to the 64-bit I/O register @a reg.
//!
//! @attention  reg is the address of the MSB register.
//! @note       MSBs are written first.
static inline void iowr64(ioreg_t reg, uint64_t mask, uint64_t data) {
    __ip_write(reg + 0, mask >> 32, data >> 32);
    __ip_write(reg + 1, mask & 0x00000000FFFFFFFFLL, data & 0x00000000FFFFFFFFLL);
}

#pragma cplusplus reset

//! @}
#endif // __ASSEMBLER__
#endif // __HWIO_H__
