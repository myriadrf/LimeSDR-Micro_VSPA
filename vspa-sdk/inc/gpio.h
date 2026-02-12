// SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
// Copyright 2017 - 2025   NXP Semiconductors

// =============================================================================
//! @file       gpio.h
//! @brief      General purpose I/O register definitions.
//! @author     NXP Semiconductors.
// =============================================================================

#ifndef __GPIO_H__
#define __GPIO_H__

//! @ingroup    GROUP_VSPA_GPIO
//! @{

// -----------------------------------------------------------------------------
// Macros:
// -----------------------------------------------------------------------------

//! @brief      Transform a general purpose input register index
//!             into an I/O register address.
//! @param      x   The general purpose input register index.
//! @return     The general purpose input register address.
#define GPI(x) ((ioreg_t)(GPIN0 + (x)))

//! @brief      Transform a general purpose output register index
//!             into an I/O register address.
//! @param      x   The general purpose output register index.
//! @return     The general purpose output register address.
#define GPO(x) ((ioreg_t)(GPOUT0 + (x)))

// -----------------------------------------------------------------------------
// Access functions:
// -----------------------------------------------------------------------------

#pragma cplusplus on

//! @brief      Read a 32-bit general purpose input register.
//!
//! @param      index   The general purpose input register index.
//! @return     The value read from the general purpose input register.
//!
//! This function returns the 32-bit value of the general purpose input register
//! @a index.
static inline uint32_t gpird(ioindex_t index) { return iord(GPI(index)); }

//! @brief      Read a 32-bit general purpose input register with a mask.
//!
//! @param      index   The general purpose input register index.
//! @param      mask    Specifies the bit mask to read with. Only bits set to 1
//!                     in the mask are read, others are returned as zero.
//! @return     The value read from the general purpose input register.
//!
//! This function returns the 32-bit value of the general purpose input register
//! @a index, masked with the bit mask @a mask.
static inline uint32_t gpird(ioindex_t index, uint32_t mask) { return iord(GPI(index), mask); }

//! @brief      Write a 32-bit general purpose input register using register for address.
//!
//! @param      index   The general purpose input register index (this value in a reg).
//! @param      data    The value to write to the register.
//! @return     This function does not return a value.
//!
//! This function writes 32 bits specified by the value
//! @a data to the general purpose input register @a index.
static inline void gpiwr32(ioindex_t index, uint32_t data) {
    uint32_t reg = GPI(index);
    __asm__ volatile("mvip [%0], %1" : : "g"(reg), "g"(data));
}

//! @brief      Read a 32-bit general purpose output register.
//!
//! @param      index   The general purpose output register index.
//! @return     The value read from the general purpose output register.
//!
//! This function returns the 32-bit value of the general purpose output
//! register @a index.
static inline uint32_t gpord(ioindex_t index) { return iord(GPO(index)); }

//! @brief      Read a 32-bit general purpose output register with a mask.
//!
//! @param      index   The general purpose output register index.
//! @param      mask    Specifies the bit mask to read with. Only bits set to 1
//!                     in the mask are read, others are returned as zero.
//! @return     The value read from the general purpose output register.
//!
//! This function returns the 32-bit value of the general purpose output
//! register @a index, masked with the bit mask @a mask.
static inline uint32_t gpord(ioindex_t index, uint32_t mask) { return iord(GPO(index), mask); }

//! @brief      Write a 32-bit general purpose output register.
//!
//! @param      index   The general purpose output register index.
//! @param      data    The value to write to the register.
//! @return     This function does not return a value.
//!
//! This function writes the value @a data to the general purpose output
//! register @a reg.
static inline void gpowr(ioindex_t index, uint32_t data) { iowr(GPO(index), data); }

//! @brief      Write a 32-bit general purpose output register with a mask.
//!
//! @param      index   The general purpose output register index.
//! @param      mask    Specifies the bit mask to write with. Only bits set to 1
//!                     in the mask are written, others are left untouched.
//! @param      data    The value to write to the register.
//! @return     This function does not return a value.
//!
//! This function writes the bits specified by the mask @a mask from the value
//! @a data to the general purpose output register @a index.
static inline void gpowr(ioindex_t index, uint32_t mask, uint32_t data) { iowr(GPO(index), mask, data); }

//! @brief      Write a 32-bit general purpose output register using register for address.
//!
//! @param      index   The general purpose output register index (this value in a reg).
//! @param      data    The value to write to the register.
//! @return     This function does not return a value.
//!
//! This function writes 32 bits specified by the value
//! @a data to the general purpose output register @a index.
static inline void gpowr32(ioindex_t index, uint32_t data) {
    uint32_t reg = GPO(index);
    __asm__ volatile("mvip [%0], %1" : : "g"(reg), "g"(data));
}

#pragma cplusplus reset
//! @}
#endif // __GPIO_H__
