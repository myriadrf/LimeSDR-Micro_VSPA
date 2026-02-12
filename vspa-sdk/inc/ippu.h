// SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
// Copyright 2017 - 2025 Copryight   NXP Semiconductors

// =============================================================================
//! @file       ippu.h
//! @brief      Inter-vector permutation processingn unit.
//! @author     NXP Semiconductors.
//! @copyright  Copryight (C) 2017 NXP Semiconductors.
// =============================================================================

#ifndef __IPPU_H__
#define __IPPU_H__

//! @addtogroup GROUP_IPPU
//! @{

// -----------------------------------------------------------------------------
//! @defgroup   GROUP_IPPU_CTRL Control Interface
//! @{
// -----------------------------------------------------------------------------

//! @brief      IPPU procedure pointer data type.
//!
//! This data type describes an IPPU procedure address.
typedef void (*ippu_proc_t)(void);

#define IPPU_TRIG_IRQ (0b1 << 22)    //!< Trigger IRQ on IPPU completion.
#define IPPU_TRIG_VCPU (0b1 << 23)   //!< Trigger VCPU on IPPU completion.
#define IPPU_MODE_16BIT (0b1 << 24)  //!< 16-bit addressing mode.
#define IPPU_TRIG_FECU (0b1 << 26)   //!< Trigger FECU on IPPU completion.
#define IPPU_TRIG_DMAC (0b1 << 27)   //!< Trigger DMA on IPPU completion.
#define IPPU_PEND_DMAC (0b001 << 29) //!< Wait for DMA completion to start IPPU.
#define IPPU_PEND_FECU (0b010 << 29) //!< Wait for FECU completion to start IPPU.
#define IPPU_PEND_NONE (0b100 << 29) //!< Start IPPU immediately.

#pragma cplusplus on

//! @brief      Request execution of an IPPU procedure.
//! @param      proc    The address of the IPPU procedure to execute.
//! @return     This function does not return a value.
//!
//! This function requests the IPPU to execute the procedure @a proc
//! immediately (provided there is no other IPPU procedure currently
//! running or pending execution).
static inline void ippu_enable(ippu_proc_t proc) { iowr(IPPUCONTROL, IPPU_PEND_NONE | IPPU_MODE_16BIT | (uint32_t)proc); }

//! @brief      Request execution of an IPPU procedure.
//! @param      proc    The address of the IPPU procedure to execute.
//! @param      mode    Combination of flags to control the IPPU procedure
//!                     execution.
//! @return     This function does not return a value.
//!
//! This function requests the IPPU to execute the procedure @a proc
//! according to the flags set in @a mode.
static inline void ippu_enable(ippu_proc_t proc, uint32_t mode) { iowr(IPPUCONTROL, mode | IPPU_MODE_16BIT | (uint32_t)proc); }

#pragma cplusplus reset

//! @brief      Abort IPPU processing.
//! @return     This function does not return a value.
//!
//! This function stops any IPPU procedure running or pending.
static inline void ippu_abort(void) { iowr(IPPURC, 0x1 << 29, 0x1 << 29); }

//! @brief      Suspend IPPU processing.
//! @return     This function does not return a value.
//!
//! This function suspends any IPPU procedure running or pending.
//!
//! @see        ippu_resume()
static inline void ippu_suspend(void) { iowr(IPPURC, 0x1 << 30, 0x1 << 30); }

//! @brief      Resume IPPU processing.
//! @return     This function does not return a value.
//!
//! This function resumes a previously suspended IPPU procedure.
//!
//! @see        ippu_abort()
static inline void ippu_resume(void) { iowr(IPPURC, 0x1 << 30, 0x0 << 30); }

//! @brief      Clear IPPU command FIFO error.
//! @return     This function does not return a value.
//!
//! This function clears the IPPU command FIFO error bit.
static inline void ippu_error_clear(void) { iowr(IPPURC, 0x1 << 31, 0x1 << 31); }

#pragma cplusplus on

//! @brief      Read the IPPU argument base address.
//! @return     The IPPU argument base address.
//!
//! This function returns the IPPU argument base address in IPPU address
//! space.
static inline uint32_t ippu_arg_base(void) { return iord(IPPUARGBASEADDR); }

//! @brief      Write the IPPU argument base address.
//! @param      base    IPPU argument base address from a VCPU address perspective (i.e. to put in IPPU space = VCPU size + IPPU
//! address).
//! @return     This function does not return a value.
//!
//! This function programs the IPPU argument base address from the address
//! @a base in VCPU address space.
static inline void ippu_arg_base(uint32_t base) { iowr(IPPUARGBASEADDR, base); }

#pragma cplusplus reset

#pragma cplusplus on

//! @brief      Reset the IPPU.
//! @return     This function does not return a value.
//!
//! This function stops any IPPU procedure currently running and clears
//! any error bit.
static inline void ippu_reset(void) { iowr(IPPURC, (0x1 << 31) | (0x1 << 29)); }

//! @brief      Reset the IPPU.
//! @param      base    IPPU argument base address in IPPU address space.
//! @return     This function does not return a value.
//!
//! This function stops any IPPU procedure currently running, clears any
//! error bit and programs the IPPU argument base address
//! using the address @a arg in the VCPU address space.
static inline void ippu_reset(uint32_t base) {
    iowr(IPPURC, (0x1 << 31) | (0x1 << 29));
    ippu_arg_base(base);
}

#pragma cplusplus reset

//! @}

// -----------------------------------------------------------------------------
//! @defgroup   GROUP_IPPU_STAT Status Interface
//! @{
// -----------------------------------------------------------------------------

//! @brief      Test if the IPPU command FIFO is full.
//! @retval     zero        The IPPU command FIFO is not full.
//! @retval     non-zero    The IPPU command FIFO is full.
static inline uint32_t ippu_is_full(void) { return iord(IPPUSTATUS, 0x1 << 20); }

//! @brief      Test if the IPPU is currently processing.
//! @retval     zero        The IPPU is not currently processing.
//! @retval     non-zero    The IPPU is currently processing.
static inline uint32_t ippu_is_busy(void) { return iord(IPPUSTATUS, 0x1 << 21); }

//! @brief      Test if the IPPU error bit is set.
//! @retval     zero        The IPPU error bit is not set.
//! @retval     non-zero    The IPPU error bit is set.
static inline uint32_t ippu_error(void) { return iord(IPPUSTATUS, 0x1 << 27); }

//! @brief      Test if the IPPU completed execution of a procedure.
//! @retval     zero        The IPPU did not complete the execution
//!                         of a procedure.
//! @retval     non-zero   The IPPU did complete the execution
//!                         of a procedure.
static inline uint32_t ippu_is_done(void) { return iord(IPPUSTATUS, 0x1 << 28); }

//! @brief      Test if the IPPU aborted a procedure.
//! @retval     zero        The IPPU did not abort a procedure.
//! @retval     non-zero    The IPPU did abort a procedure.
static inline uint32_t ippu_is_aborted(void) { return iord(IPPUSTATUS, 0x1 << 29); }

//! @brief      Test if an IPPU procedure is suspended.
//! @retval     zero        No IPPU procedure is suspended.
//! @retval     non-zero    An IPPU procedure is suspended.
static inline uint32_t ippu_is_suspended(void) { return iord(IPPUSTATUS, 0x1 << 30); }

//! @brief      Test if the IPPU is active.
//! @retval     zero        The IPPU is not active.
//! @retval     non-zero    The IPPU is active.
//!
//! This function returns non-zero when IPPU is busy or there is at least
//! one entry in the command FIFO.
//! This function returns zero when IPPU is not busy and the command FIFO
//! is empty.
static inline uint32_t ippu_is_active(void) { return iord(IPPUSTATUS, 0x1 << 31); }

extern uint32_t volatile ippu_args[];
extern uint32_t volatile ippu_arg[];

extern uint32_t ippu_base[];

static inline uint32_t ippu_dmem_base(void) { return vcpu_dmem_size(); }

#define IPPU_OFFSET(addr) ((uint32_t)(addr)-ippu_dmem_base())

#pragma cplusplus on
static inline uint32_t ippu_swver(void) { return iord(IPPUSWVER); }

static inline void ippu_swver(uint32_t data) { iowr(IPPUSWVER, data); }
#pragma cplusplus reset

//! @}
//! @}

#endif // __IPPU_H__
