// SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
// Copyright 2023 - 2025  NXP

// =============================================================================
//! @file       axiq-LA9310.h
//! @brief      LA9310 AXI I/Q FIFO driver interface.
//! @author     NXP Semiconductors.
// =============================================================================

#ifndef __AXIQ_LA9310_H__
#define __AXIQ_LA9310_H__

// -----------------------------------------------------------------------------
// AXIQ banks and FIFOs:
// -----------------------------------------------------------------------------

//! @brief      AXIQ bank enumeration.
enum axiq_bank_e {
    AXIQ_BANK_0 = 0, //!< AXIQ bank 0.
    AXIQ_BANK_CNT    //!< Total number of AXIQ banks.
};

//! @brief      AXIQ FIFO enumeration.
enum axiq_fifo_e {
    AXIQ_FIFO_RX0 = 0,  //!< AXIQ FIFO RX0
    AXIQ_FIFO_RX1 = 1,  //!< AXIQ FIFO RX1
    AXIQ_FIFO_RX2 = 2,  //!< AXIQ FIFO RO0
    AXIQ_FIFO_RX3 = 3,  //!< AXIQ FIFO TO1
    AXIQ_FIFO_TX0 = 10, //!< AXIQ FIFO TX1
    AXIQ_FIFO_CNT       //!< Total number of FIFO per AXIQ bank.
};

//! @brief      DMA channel for a given FIFO in an AXIQ bank.
//! @param      bank    specifies the AXIQ bank.
//! @param      fifo    specifies the AXIQ FIFO.
//! @return     The DMA channel number for the FIFO @a fifo in the AXIQ bank
//!             @a bank.
#define AXIQ_CHAN(bank, fifo) (((bank)*AXIQ_FIFO_CNT) + (fifo) + 1)

//! @brief      DMA channel mask for a given RX FIFO.
//! @param      bank    specifies the AXIQ bank.
//! @param      fifo    specifies the AXIQ RX FIFO.
//! @return     The DMA channel number for the RX FIFO @a fifo.
#define AXIQ_MASK(bank, fifo) (0b1 << AXIQ_CHAN(bank, fifo))

//! @brief      AXI address for a given FIFO in an AXIQ bank.
//! @param      bank    specifies the AXIQ bank.
//! @param      fifo    specifies the AXIQ FIFO.
//! @return     The AXI address for the FIFO @a fifo in the AXIQ bank @a bank.
#define AXIQ_ADDR(bank, fifo) (BASE_AXIQ(bank) + (((fifo) + 1) * 0x1000))

// -----------------------------------------------------------------------------
// Control register:
// -----------------------------------------------------------------------------

#define AXIQ_CR0 GPO(GPIO_AXIQ_CR0) //!< AXIQ control register 0.
#define AXIQ_CR1 GPO(GPIO_AXIQ_CR1) //!< AXIQ control register 1.
#define AXIQ_CR2 GPO(GPIO_AXIQ_CR2) //!< AXIQ control register 2.
#define AXIQ_CR3 GPO(GPIO_AXIQ_CR3) //!< AXIQ control register 3.

#define AXIQ_CR_SHIFT_RX0 0  //!< Bit position for FIFO RX0 in control register.
#define AXIQ_CR_SHIFT_RX1 8  //!< Bit position for FIFO RX1 in control register.
#define AXIQ_CR_SHIFT_RX2 16 //!< Bit position for FIFO RX2 in control register.
#define AXIQ_CR_SHIFT_RX3 24 //!< Bit position for FIFO RX3 in control register.
#define AXIQ_CR_SHIFT_TX0 0  //!< Bit position for FIFO TX0 in control register.

#define AXIQ_CR_FIELD_FIFO 0x1F                                     //!< Bit mask for a FIFO in control register.
#define AXIQ_CR_FIELD_RX0 (AXIQ_CR_FIELD_FIFO << AXIQ_CR_SHIFT_RX0) //!< Bit mask of FIFO RX0 in control register.
#define AXIQ_CR_FIELD_RX1 (AXIQ_CR_FIELD_FIFO << AXIQ_CR_SHIFT_RX1) //!< Bit mask of FIFO RX1 in control register.
#define AXIQ_CR_FIELD_RX2 (AXIQ_CR_FIELD_FIFO << AXIQ_CR_SHIFT_RX2) //!< Bit mask of FIFO RX2 in control register.
#define AXIQ_CR_FIELD_RX3 (AXIQ_CR_FIELD_FIFO << AXIQ_CR_SHIFT_RX3) //!< Bit mask of FIFO RX3 in control register.
#define AXIQ_CR_FIELD_TX0 (AXIQ_CR_FIELD_FIFO << AXIQ_CR_SHIFT_TX0) //!< Bit mask of FIFO TX0 in control register.

#define AXIQ_CR_SHIFT_ENABLE 0  //!< Enable bit position in control register.
#define AXIQ_CR_SHIFT_IQ_SWAP 3 //!< IQ swap bit position in control register.
#define AXIQ_CR_SHIFT_CLRERR 4  //!< Clear error bit position in control register.

#define AXIQ_CR_FIELD_ENABLE (0b1 << AXIQ_CR_SHIFT_ENABLE)   //!< Enable bit mask in control register.
#define AXIQ_CR_FIELD_IQ_SWAP (0b1 << AXIQ_CR_SHIFT_IQ_SWAP) //!< IQ swap bit mask in control register.
#define AXIQ_CR_FIELD_CLRERR (0b1 << AXIQ_CR_SHIFT_CLRERR)   //!< Clear error bit mask in control register.

#define AXIQ_CR_DISABLE (0b0 << AXIQ_CR_SHIFT_ENABLE)      //!< Value to disable a FIFO.
#define AXIQ_CR_ENABLE (0b1 << AXIQ_CR_SHIFT_ENABLE)       //!< Value to enable a FIFO.
#define AXIQ_CR_IQ_SWAP_ON (0b1 << AXIQ_CR_SHIFT_IQ_SWAP)  //!< Value to enable IQ swap.
#define AXIQ_CR_IQ_SWAP_OFF (0b0 << AXIQ_CR_SHIFT_IQ_SWAP) //!< Value to disable IQ swap.

#define AXIQ_CR_CLRERR (0b1 << AXIQ_CR_SHIFT_CLRERR) //!< Value to clear FIFO errors.

#define AXIQ_CR_FIELD_ENABLE_RX_ALL                                                              \
    ((AXIQ_CR_FIELD_ENABLE << AXIQ_CR_SHIFT_RX0) | (AXIQ_CR_FIELD_ENABLE << AXIQ_CR_SHIFT_RX1) | \
     (AXIQ_CR_FIELD_ENABLE << AXIQ_CR_SHIFT_RX2) | (AXIQ_CR_FIELD_ENABLE << AXIQ_CR_SHIFT_RX3))

#define AXIQ_CR_FIELD_CLRERR_RX_ALL                                                              \
    ((AXIQ_CR_FIELD_CLRERR << AXIQ_CR_SHIFT_RX0) | (AXIQ_CR_FIELD_CLRERR << AXIQ_CR_SHIFT_RX1) | \
     (AXIQ_CR_FIELD_CLRERR << AXIQ_CR_SHIFT_RX2) | (AXIQ_CR_FIELD_CLRERR << AXIQ_CR_SHIFT_RX3))

#define AXIQ_CR_FIELD_IQ_SWAP_RX_ALL                                                               \
    ((AXIQ_CR_FIELD_IQ_SWAP << AXIQ_CR_SHIFT_RX0) | (AXIQ_CR_FIELD_IQ_SWAP << AXIQ_CR_SHIFT_RX1) | \
     (AXIQ_CR_FIELD_IQ_SWAP << AXIQ_CR_SHIFT_RX2) | (AXIQ_CR_FIELD_IQ_SWAP << AXIQ_CR_SHIFT_RX3))

#define AXIQ_CR_DISABLE_RX_ALL                                                                                                  \
    ((AXIQ_CR_DISABLE << AXIQ_CR_SHIFT_RX0) | (AXIQ_CR_DISABLE << AXIQ_CR_SHIFT_RX1) | (AXIQ_CR_DISABLE << AXIQ_CR_SHIFT_RX2) | \
     (AXIQ_CR_DISABLE << AXIQ_CR_SHIFT_RX3))

#define AXIQ_CR_ENABLE_RX_ALL                                                                                                \
    ((AXIQ_CR_ENABLE << AXIQ_CR_SHIFT_RX0) | (AXIQ_CR_ENABLE << AXIQ_CR_SHIFT_RX1) | (AXIQ_CR_ENABLE << AXIQ_CR_SHIFT_RX2) | \
     (AXIQ_CR_ENABLE << AXIQ_CR_SHIFT_RX3))

#define AXIQ_CR_CLRERR_RX_ALL                                                                                                \
    ((AXIQ_CR_CLRERR << AXIQ_CR_SHIFT_RX0) | (AXIQ_CR_CLRERR << AXIQ_CR_SHIFT_RX1) | (AXIQ_CR_CLRERR << AXIQ_CR_SHIFT_RX2) | \
     (AXIQ_CR_CLRERR << AXIQ_CR_SHIFT_RX3))

#define AXIQ_CR_IQ_SWAP_RX_ALL_ON                                                            \
    ((AXIQ_CR_IQ_SWAP_ON << AXIQ_CR_SHIFT_RX0) | (AXIQ_CR_IQ_SWAP_ON << AXIQ_CR_SHIFT_RX1) | \
     (AXIQ_CR_IQ_SWAP_ON << AXIQ_CR_SHIFT_RX2) | (AXIQ_CR_IQ_SWAP_ON << AXIQ_CR_SHIFT_RX3))

#define AXIQ_CR_IQ_SWAP_RX_ALL_OFF                                                             \
    ((AXIQ_CR_IQ_SWAP_OFF << AXIQ_CR_SHIFT_RX0) | (AXIQ_CR_IQ_SWAP_OFF << AXIQ_CR_SHIFT_RX1) | \
     (AXIQ_CR_IQ_SWAP_OFF << AXIQ_CR_SHIFT_RX2) | (AXIQ_CR_IQ_SWAP_OFF << AXIQ_CR_SHIFT_RX3))

//! @brief      FIFO bit field position in an AXIQ control register.
//! @param      fifo    specifies the FIFO.
//! @return     The position of the LSB of the FIFO bit field.
static inline uint32_t axiq_cr_shift(enum axiq_fifo_e fifo) {
    switch (fifo) {
    case AXIQ_FIFO_RX0:
        return AXIQ_CR_SHIFT_RX0;

    case AXIQ_FIFO_RX1:
        return AXIQ_CR_SHIFT_RX1;

    case AXIQ_FIFO_RX2:
        return AXIQ_CR_SHIFT_RX2;

    case AXIQ_FIFO_RX3:
        return AXIQ_CR_SHIFT_RX3;

    case AXIQ_FIFO_TX0:
        return AXIQ_CR_SHIFT_TX0;

    default:
        __swbreak();
        return 0;
    }
}

// -----------------------------------------------------------------------------
// Status register:
// -----------------------------------------------------------------------------

#define AXIQ_SR0 GPI(GPIO_AXIQ_SR0) //!< AXIQ status register 0.
#define AXIQ_SR1 GPI(GPIO_AXIQ_SR1) //!< AXIQ status register 1.
#define AXIQ_SR2 GPI(GPIO_AXIQ_SR2) //!< AXIQ status register 2.
#define AXIQ_SR3 GPI(GPIO_AXIQ_SR3) //!< AXIQ status register 3.

#define AXIQ_SR_SHIFT_RX0 0  //!< Bit position for FIFO RX0 in status register.
#define AXIQ_SR_SHIFT_RX1 4  //!< Bit position for FIFO RX1 in status register.
#define AXIQ_SR_SHIFT_RX2 8  //!< Bit position for FIFO RX2 in status register.
#define AXIQ_SR_SHIFT_RX3 12 //!< Bit position for FIFO RX3 in status register.
#define AXIQ_SR_SHIFT_TX0 16 //!< Bit position for FIFO TX0 in status register.

#define AXIQ_SR_FIELD_FIFO 0xF                                      //!< Bit mask for a FIFO in status register.
#define AXIQ_SR_FIELD_RX0 (AXIQ_SR_FIELD_FIFO << AXIQ_SR_SHIFT_RX0) //!< Bit mask of FIFO RX0 in control register.
#define AXIQ_SR_FIELD_RX1 (AXIQ_SR_FIELD_FIFO << AXIQ_SR_SHIFT_RX1) //!< Bit mask of FIFO RX1 in control register.
#define AXIQ_SR_FIELD_RX2 (AXIQ_SR_FIELD_FIFO << AXIQ_SR_SHIFT_RX2) //!< Bit mask of FIFO RX2 in control register.
#define AXIQ_SR_FIELD_RX3 (AXIQ_SR_FIELD_FIFO << AXIQ_SR_SHIFT_RX3) //!< Bit mask of FIFO RX3 in control register.
#define AXIQ_SR_FIELD_TX0 (AXIQ_SR_FIELD_FIFO << AXIQ_SR_SHIFT_TX0) //!< Bit mask of FIFO TX0 in control register.

#define AXIQ_SR_SHIFT_ENABLED 0  //!< Enable bit position in status register.
#define AXIQ_SR_SHIFT_NOTEMPTY 1 //!< FIFO not empty bit position in status register.
#define AXIQ_SR_SHIFT_NOTFULL 1  //!< FIFO not full bit position in status register.
#define AXIQ_SR_SHIFT_ERRUNDER 2 //!< FIFO underrun error bit position in status register.
#define AXIQ_SR_SHIFT_ERROVER 3  //!< FIFO overrun error bit position in status register.

#define AXIQ_SR_FIELD_ENABLED (0b1 << AXIQ_SR_SHIFT_ENABLED)   //!< Enable bit mask in status register.
#define AXIQ_SR_FIELD_NOTEMPTY (0b1 << AXIQ_SR_SHIFT_NOTEMPTY) //!< FIFO not empty bit mask in status register.
#define AXIQ_SR_FIELD_NOTFULL (0b1 << AXIQ_SR_SHIFT_NOTFULL)   //!< FIFO not full bit mask in status register.
#define AXIQ_SR_FIELD_ERRUNDER (0b1 << AXIQ_SR_SHIFT_ERRUNDER) //!< FIFO underrun error bit mask in status register.
#define AXIQ_SR_FIELD_ERROVER (0b1 << AXIQ_SR_SHIFT_ERROVER)   //!< FIFO overrun error bit mask in status register.

#define AXIQ_SR_FIELD_ERRUNDER_RX_ALL                                                                \
    ((AXIQ_SR_FIELD_ERRUNDER << AXIQ_SR_SHIFT_RX0) | (AXIQ_SR_FIELD_ERRUNDER << AXIQ_SR_SHIFT_RX1) | \
     (AXIQ_SR_FIELD_ERRUNDER << AXIQ_SR_SHIFT_RX2) | (AXIQ_SR_FIELD_ERRUNDER << AXIQ_SR_SHIFT_RX3))

#define AXIQ_SR_FIELD_ERROVER_RX_ALL                                                               \
    ((AXIQ_SR_FIELD_ERROVER << AXIQ_SR_SHIFT_RX0) | (AXIQ_SR_FIELD_ERROVER << AXIQ_SR_SHIFT_RX1) | \
     (AXIQ_SR_FIELD_ERROVER << AXIQ_SR_SHIFT_RX2) | (AXIQ_SR_FIELD_ERROVER << AXIQ_SR_SHIFT_RX3))

//! @brief      AXIQ FIFO status bit field shift value.
//! @param      fifo    specifies the AXIQ FIFO.
//! @return     The number of shifts required to align a FIFO status bit field.
static inline uint32_t axiq_sr_shift(enum axiq_fifo_e fifo) {
    switch (fifo) {
    case AXIQ_FIFO_RX0:
        return AXIQ_SR_SHIFT_RX0;

    case AXIQ_FIFO_RX1:
        return AXIQ_SR_SHIFT_RX1;

    case AXIQ_FIFO_RX2:
        return AXIQ_SR_SHIFT_RX2;

    case AXIQ_FIFO_RX3:
        return AXIQ_SR_SHIFT_RX3;

    case AXIQ_FIFO_TX0:
        return AXIQ_SR_SHIFT_TX0;

    default:
        __swbreak();
        return 0;
    }
}

/*
 *
 * axiq_ls stub
 *
 *
 * */

// =============================================================================
// Physical driver interface:
// =============================================================================
// -----------------------------------------------------------------------------
// RX FIFO-level access:
// -----------------------------------------------------------------------------

#pragma cplusplus on

//! @brief      Read the RX FIFO control bits from the AXIQ control register.
//! @param      bank    specifies the AXIQ bank.
//! @param      fifo    specifies the RX FIFO.
//! @return     The in-place value of the RX FIFO control bits.
static inline uint32_t axiq_fifo_rx_cr(enum axiq_bank_e bank, enum axiq_fifo_e fifo) {
    return iord(AXIQ_CR0, AXIQ_CR_FIELD_FIFO << axiq_cr_shift(fifo));
}

//! @brief      Write the RX FIFO control bits in the AXIQ control register.
//! @param      bank    specifies the AXIQ bank.
//! @param      fifo    specifies the RX FIFO.
//! @param      ctrl    the right-justified control value to write.
//! @return     This function does not return a value.
static inline void axiq_fifo_rx_cr(enum axiq_bank_e bank, enum axiq_fifo_e fifo, uint32_t ctrl) {
    iowr(AXIQ_CR0, AXIQ_CR_FIELD_FIFO << axiq_cr_shift(fifo), ctrl << axiq_cr_shift(fifo));
}

//! @brief      Write the RX FIFO control bits in the AXIQ control register.
//! @param      bank    specifies the AXIQ bank.
//! @param      fifo    specifies the RX FIFO.
//! @param      mask    right-justified bit mask specifying the control bits to write.
//! @param      ctrl    the right-justified control value to write.
//! @return     This function does not return a value.
static inline void axiq_fifo_rx_cr(enum axiq_bank_e bank, enum axiq_fifo_e fifo, uint32_t mask, uint32_t ctrl) {
    iowr(AXIQ_CR0, (AXIQ_CR_FIELD_FIFO & mask) << axiq_cr_shift(fifo), ctrl << axiq_cr_shift(fifo));
}

//! @brief      Read an AXIQ RX FIFO satus field.
//! @param      bank    specifies the AXIQ bank.
//! @param      fifo    specifies the AXIQ RX FIFO.
//! @return     The in-place value of the AXIQ RX FIFO status field.
static inline uint32_t axiq_fifo_rx_sr(enum axiq_bank_e bank, enum axiq_fifo_e fifo) {
    return iord(AXIQ_SR0, AXIQ_SR_FIELD_FIFO << axiq_sr_shift(fifo));
}

//! @brief      Read an AXIQ RX FIFO status bit field.
//! @param      bank    specifies the AXIQ bank.
//! @param      fifo    specifies the AXIQ RX FIFO.
//! @param      mask    specifies the bit field mask.
//! @return     The in-place value of the AXIQ RX FIFO status bit field.
static inline uint32_t axiq_fifo_rx_sr(enum axiq_bank_e bank, enum axiq_fifo_e fifo, uint32_t mask) {
    return iord(AXIQ_SR0, (AXIQ_SR_FIELD_FIFO & mask) << axiq_sr_shift(fifo));
}

#pragma cplusplus reset

// -----------------------------------------------------------------------------
// TX FIFO-level access:
// -----------------------------------------------------------------------------

#pragma cplusplus on

//! @brief      Read the TX FIFO control bits from the AXIQ control register.
//! @param      bank    specifies the AXIQ bank.
//! @param      fifo    specifies the TX FIFO.
//! @return     The in-place value of the TX FIFO control bits.
static inline uint32_t axiq_fifo_tx_cr(enum axiq_bank_e bank, enum axiq_fifo_e fifo) {
    return iord(AXIQ_CR3, AXIQ_CR_FIELD_FIFO << axiq_cr_shift(fifo));
}

//! @brief      Write the TX FIFO control bits in the AXIQ control register.
//! @param      bank    specifies the AXIQ bank.
//! @param      fifo    specifies the TX FIFO.
//! @param      ctrl    the right-justified control value to write.
//! @return     This function does not return a value.
static inline void axiq_fifo_tx_cr(enum axiq_bank_e bank, enum axiq_fifo_e fifo, uint32_t ctrl) {
    iowr(AXIQ_CR3, AXIQ_CR_FIELD_FIFO << axiq_cr_shift(fifo), ctrl << axiq_cr_shift(fifo));
}

//! @brief      Write the TX FIFO control bits in the AXIQ control register.
//! @param      bank    specifies the AXIQ bank.
//! @param      fifo    specifies the TX FIFO.
//! @param      mask    right-justified bit mask specifying the control bits to write.
//! @param      ctrl    the right-justified control value to write.
//! @return     This function does not return a value.
static inline void axiq_fifo_tx_cr(enum axiq_bank_e bank, enum axiq_fifo_e fifo, uint32_t mask, uint32_t ctrl) {
    iowr(AXIQ_CR3, (AXIQ_CR_FIELD_FIFO & mask) << axiq_cr_shift(fifo), ctrl << axiq_cr_shift(fifo));
}

//! @brief      Read an AXIQ TX FIFO satus field.
//! @param      bank    specifies the AXIQ bank.
//! @param      fifo    specifies the AXIQ TX FIFO.
//! @return     The in-place value of the AXIQ TX FIFO status field.
static inline uint32_t axiq_fifo_tx_sr(enum axiq_bank_e bank, enum axiq_fifo_e fifo) {
    return iord(AXIQ_SR1, AXIQ_SR_FIELD_FIFO << axiq_sr_shift(fifo));
}

//! @brief      Read an AXIQ TX FIFO status bit field.
//! @param      bank    specifies the AXIQ bank.
//! @param      fifo    specifies the AXIQ TX FIFO.
//! @param      mask    specifies the bit field mask.
//! @return     The in-place value of the AXIQ TX FIFO status bit field.
static inline uint32_t axiq_fifo_tx_sr(enum axiq_bank_e bank, enum axiq_fifo_e fifo, uint32_t mask) {
    return iord(AXIQ_SR1, (AXIQ_SR_FIELD_FIFO & mask) << axiq_sr_shift(fifo));
}

#pragma cplusplus reset

// =============================================================================
// Logical driver interface:
// =============================================================================

// -----------------------------------------------------------------------------
// Bank-level interface:
// -----------------------------------------------------------------------------

//! @brief      Disable all RX FIFOs in an AXIQ bank.
//! @return     This function does not return a value.
static inline void axiq_rx_disable() {
    // Disable RX FIFOs:
    iowr(AXIQ_CR0, AXIQ_CR_FIELD_ENABLE_RX_ALL, AXIQ_CR_DISABLE_RX_ALL);
}

//! @brief      Disable all TX FIFOs in an AXIQ bank.
//! @return     This function does not return a value.
static inline void axiq_tx_disable() {
    // Disable TX FIFOs:
    iowr(AXIQ_CR3, (AXIQ_CR_FIELD_ENABLE << AXIQ_CR_SHIFT_TX0), (AXIQ_CR_DISABLE << AXIQ_CR_SHIFT_TX0));
}

//! @brief      Disable all FIFOs in an AXIQ bank.
//! @return     This function does not return a value.
static inline void axiq_disable() {
    axiq_rx_disable();
    axiq_tx_disable();
}

//! @brief      Enable all RX FIFOs in an AXIQ bank.
//! @return     This function does not return a value.
static inline void axiq_rx_enable() {
    // ENABLE RX FIFOs:
    iowr(AXIQ_CR0, AXIQ_CR_FIELD_ENABLE_RX_ALL, AXIQ_CR_ENABLE_RX_ALL);
}
//! @brief      Enable all TX FIFOs in an AXIQ bank.
//! @return     This function does not return a value.
static inline void axiq_tx_enable() {

    // ENABLE TX FIFOs:
    iowr(AXIQ_CR3, (AXIQ_CR_FIELD_ENABLE << AXIQ_CR_SHIFT_TX0), (AXIQ_CR_ENABLE << AXIQ_CR_SHIFT_TX0));
}

//! @brief      Enable IQ SWAP on all RX FIFOs in an AXIQ bank.
//! @return     This function does not return a value.
static inline void axiq_rx_iq_swap_on() {
    // ENABLE RX FIFOs:
    iowr(AXIQ_CR0, AXIQ_CR_FIELD_IQ_SWAP_RX_ALL, AXIQ_CR_IQ_SWAP_RX_ALL_ON);
}

//! @brief      Disable IQ SWAP on all RX FIFOs in an AXIQ bank.
//! @return     This function does not return a value.
static inline void axiq_rx_iq_swap_off() {
    // ENABLE RX FIFOs:
    iowr(AXIQ_CR0, AXIQ_CR_FIELD_IQ_SWAP_RX_ALL, AXIQ_CR_IQ_SWAP_RX_ALL_OFF);
}

//! @brief      Enable all FIFOs in an AXIQ bank.
//! @return     This function does not return a value.
static inline void axiq_enable() {
    // ENABLE RX FIFOs:
    axiq_rx_enable();

    // ENABLE TX FIFOs:
    axiq_tx_enable();
}

//! @brief      AXIQ bank status data structure.
struct axiq_bank_status_t {
    uint32_t rx; //!< RX FIFOs status.
    uint32_t tx; //!< TX FIFOs status.
};

static inline struct axiq_bank_status_t axiq_bank_status(enum axiq_bank_e bank) {
    struct axiq_bank_status_t status;
    status.rx = iord(AXIQ_SR0);
    status.tx = iord(AXIQ_SR1);
    return status;
}

//! @brief      AXIQ bank error data structure.
struct axiq_bank_error_t {
    uint32_t rx; //!< RX FIFOs errors.
    uint32_t tx; //!< TX FIFOs errors.
};

static inline struct axiq_bank_error_t axiq_bank_error(enum axiq_bank_e bank) {
    struct axiq_bank_error_t error;
    error.rx = iord(AXIQ_SR0, AXIQ_SR_FIELD_ERRUNDER_RX_ALL | AXIQ_SR_FIELD_ERROVER_RX_ALL);
    error.tx = iord(AXIQ_SR1, (AXIQ_SR_FIELD_ERRUNDER << AXIQ_SR_SHIFT_TX0) | (AXIQ_SR_FIELD_ERROVER << AXIQ_SR_SHIFT_TX0));
    return error;
}

static inline void axiq_bank_clrerr(enum axiq_bank_e bank) {
    // Clear RX FIFOs errors:
    iowr(AXIQ_CR0, AXIQ_CR_FIELD_CLRERR_RX_ALL, AXIQ_CR_CLRERR_RX_ALL);
    // Clear TX FIFOs errors:
    iowr(AXIQ_CR3, (AXIQ_CR_FIELD_CLRERR << AXIQ_CR_SHIFT_TX0), (AXIQ_CR_CLRERR << AXIQ_CR_SHIFT_TX0));
}

// -----------------------------------------------------------------------------
// FIFO-level interface:
// -----------------------------------------------------------------------------

//! @brief      AXIQ FIFO descriptor.
struct axiq_fifo_t {
    uint32_t addr; //!< FIFO address.
    uint32_t chan; //!< DMA channel number.
    uint32_t mask; //!< DMA channel bit mask.
};

//! @brief      Initialize an AXIQ RX FIFO descriptor.
//! @param      axiq    the AXIQ FIFO descriptor.
//! @param      bank    specifies the AXIQ bank.
//! @param      fifo    specifies the AXIQ FIFO.
//! @return     This function does not return a value.
static inline void axiq_fifo_open(struct axiq_fifo_t *axiq, enum axiq_bank_e bank, enum axiq_fifo_e fifo) {
    axiq->addr = AXIQ_ADDR(bank, fifo);
    axiq->chan = AXIQ_CHAN(bank, fifo);
    axiq->mask = AXIQ_MASK(bank, fifo);
}

static inline void axiq_fifo_rx_disable(enum axiq_bank_e bank, enum axiq_fifo_e fifo) {
    axiq_fifo_rx_cr(bank, fifo, AXIQ_CR_FIELD_ENABLE, AXIQ_CR_DISABLE);
}

static inline void axiq_fifo_tx_disable(enum axiq_bank_e bank, enum axiq_fifo_e fifo) {
    axiq_fifo_tx_cr(bank, fifo, AXIQ_CR_FIELD_ENABLE, AXIQ_CR_DISABLE);
}

static inline void axiq_fifo_rx_enable(enum axiq_bank_e bank, enum axiq_fifo_e fifo) {
    axiq_fifo_rx_cr(bank, fifo, AXIQ_CR_FIELD_ENABLE, AXIQ_CR_ENABLE);
}

static inline void axiq_fifo_tx_enable(enum axiq_bank_e bank, enum axiq_fifo_e fifo) {
    axiq_fifo_tx_cr(bank, fifo, AXIQ_CR_FIELD_ENABLE, AXIQ_CR_ENABLE);
}

static inline void axiq_fifo_rx_iq_swap_on(enum axiq_bank_e bank, enum axiq_fifo_e fifo) {
    axiq_fifo_rx_cr(bank, fifo, AXIQ_CR_FIELD_IQ_SWAP, AXIQ_CR_IQ_SWAP_ON);
}

static inline void axiq_fifo_tx_iq_swap_on(enum axiq_bank_e bank, enum axiq_fifo_e fifo) {
    axiq_fifo_tx_cr(bank, fifo, AXIQ_CR_FIELD_IQ_SWAP, AXIQ_CR_IQ_SWAP_ON);
}

static inline void axiq_fifo_rx_iq_swap_off(enum axiq_bank_e bank, enum axiq_fifo_e fifo) {
    axiq_fifo_rx_cr(bank, fifo, AXIQ_CR_FIELD_IQ_SWAP, AXIQ_CR_IQ_SWAP_OFF);
}

static inline void axiq_fifo_tx_iq_swap_off(enum axiq_bank_e bank, enum axiq_fifo_e fifo) {
    axiq_fifo_tx_cr(bank, fifo, AXIQ_CR_FIELD_IQ_SWAP, AXIQ_CR_IQ_SWAP_OFF);
}

static inline void axiq_fifo_rx_clrerr(enum axiq_bank_e bank, enum axiq_fifo_e fifo) {
    axiq_fifo_rx_cr(bank, fifo, AXIQ_CR_FIELD_CLRERR, AXIQ_CR_CLRERR);
}

static inline void axiq_fifo_tx_clrerr(enum axiq_bank_e bank, enum axiq_fifo_e fifo) {
    axiq_fifo_tx_cr(bank, fifo, AXIQ_CR_FIELD_CLRERR, AXIQ_CR_CLRERR);
}

static inline uint32_t axiq_fifo_rx_status(enum axiq_bank_e bank, enum axiq_fifo_e fifo) { return axiq_fifo_rx_sr(bank, fifo); }

static inline uint32_t axiq_fifo_tx_status(enum axiq_bank_e bank, enum axiq_fifo_e fifo) { return axiq_fifo_tx_sr(bank, fifo); }

static inline uint32_t axiq_fifo_rx_error(enum axiq_bank_e bank, enum axiq_fifo_e fifo) {
    return axiq_fifo_rx_sr(bank, fifo, AXIQ_SR_FIELD_ERRUNDER | AXIQ_SR_FIELD_ERROVER);
}

static inline uint32_t axiq_fifo_tx_error(enum axiq_bank_e bank, enum axiq_fifo_e fifo) {
    return axiq_fifo_tx_sr(bank, fifo, AXIQ_SR_FIELD_ERRUNDER | AXIQ_SR_FIELD_ERROVER);
}

#pragma cplusplus on

#define AXIQ_FIFO_RD (DMAC_RDC | DMAC_FIFO)
#define AXIQ_FIFO_WR (DMAC_WRC | DMAC_FIFO)

static inline void axiq_fifo_rx_read(enum axiq_bank_e bank, enum axiq_fifo_e fifo, uint32_t ctrl, void *data, size_t size) {
    dmac_enable(ctrl | AXIQ_FIFO_RD | AXIQ_CHAN(bank, fifo), size, AXIQ_ADDR(bank, fifo), data);
}

static inline void axiq_fifo_rx_read(enum axiq_bank_e bank, enum axiq_fifo_e fifo, void *data, size_t size) {
    dmac_enable(DMAC_TRIG_VCPU | AXIQ_FIFO_RD | AXIQ_CHAN(bank, fifo), size, AXIQ_ADDR(bank, fifo), data);
}

static inline void axiq_fifo_rx_read(struct axiq_fifo_t *axiq, uint32_t ctrl, void *data, size_t size) {
    dmac_enable(ctrl | AXIQ_FIFO_RD | axiq->chan, size, axiq->addr, data);
}
static inline void axiq_fifo_tx_write(enum axiq_bank_e bank, enum axiq_fifo_e fifo, uint32_t ctrl, void const *data, size_t size) {
    dmac_enable(ctrl | AXIQ_FIFO_WR | AXIQ_CHAN(bank, fifo), size, AXIQ_ADDR(bank, fifo), (void *)data);
}

static inline void axiq_fifo_tx_write(enum axiq_bank_e bank, enum axiq_fifo_e fifo, void const *data, size_t size) {
    dmac_enable(DMAC_TRIG_VCPU | AXIQ_FIFO_WR | AXIQ_CHAN(bank, fifo), size, AXIQ_ADDR(bank, fifo), (void *)data);
}

#pragma cplusplus reset

#endif // __AXIQ_LA9310_H__
