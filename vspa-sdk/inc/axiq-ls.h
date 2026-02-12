// SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
// Copyright 2019 - 2025   NXP Semiconductors

// =============================================================================
//! @file       axiq-ls.h
//! @brief      Low-speed AXIQ driver.
//! @author     NXP Semiconductors.
// =============================================================================

#ifndef __AXIQ_LS_H__
#define __AXIQ_LS_H__

#include "gpio.h"
#include "dmac.h"
#include "axiq-la1224.h"
// =============================================================================
// AXIQ FIFOs:
// =============================================================================

//! @brief      AXIQ RX FIFO enumeration.
enum axiq_ls_fifo_rx_e {
    AXIQ_LS_FIFO_RX0,   //!< AXIQ FIFO RX0.
    AXIQ_LS_FIFO_RX1,   //!< AXIQ FIFO RX1.
    AXIQ_LS_FIFO_RX2,   //!< AXIQ FIFO RX2.
    AXIQ_LS_FIFO_RX_CNT //!< Total number of RX FIFO.
};

//! @brief      AXIQ TX FIFO enumeration.
enum axiq_ls_fifo_tx_e {
    AXIQ_LS_FIFO_TX0,   //!< AXIQ FIFO TX0.
    AXIQ_LS_FIFO_TX1,   //!< AXIQ FIFO TX1.
    AXIQ_LS_FIFO_TX_CNT //!< Total number of TX FIFO.
};

// =============================================================================
// Control register:
// =============================================================================

//! @brief      Number of right-shifts to right-justify the FIFO control bits.
#define AXIQ_LS_CR_SHIFT(fifo) ((fifo)*8)

// RX FIFO control register (CR0) definitions:
#define AXIQ_LS_CR_SHIFT_RX0 0
#define AXIQ_LS_CR_SHIFT_RX1 8
#define AXIQ_LS_CR_SHIFT_RX2 16

#define AXIQ_LS_CR_FIELD_RX0 (0xFF << AXIQ_LS_CR_SHIFT_RX0)
#define AXIQ_LS_CR_FIELD_RX1 (0xFF << AXIQ_LS_CR_SHIFT_RX1)
#define AXIQ_LS_CR_FIELD_RX2 (0x3F << AXIQ_LS_CR_SHIFT_RX2)

// TX FIFO control register (CR1) definitions:
#define AXIQ_LS_CR_SHIFT_TX0 0
#define AXIQ_LS_CR_SHIFT_TX1 8

#define AXIQ_LS_CR_FIELD_TX0 (0x7F << AXIQ_LS_CR_SHIFT_TX0)
#define AXIQ_LS_CR_FIELD_TX1 (0x3F << AXIQ_LS_CR_SHIFT_TX1)

// FIFO control register definitions:
#define AXIQ_LS_CR_SHIFT_ENABLE 0
#define AXIQ_LS_CR_SHIFT_IQSWAP 3
#define AXIQ_LS_CR_SHIFT_CLRERR 4
#define AXIQ_LS_CR_SHIFT_RESET 5
#define AXIQ_LS_CR_SHIFT_COMPLEX 6
#define AXIQ_LS_CR_SHIFT_NONINT 7

#define AXIQ_LS_CR_FIELD_FIFO 0xFF
#define AXIQ_LS_CR_FIELD_ENABLE (0b1 << AXIQ_LS_CR_SHIFT_ENABLE)
#define AXIQ_LS_CR_FIELD_DISABLE (0b0 << AXIQ_LS_CR_SHIFT_ENABLE)
#define AXIQ_LS_CR_FIELD_IQSWAP (0b1 << AXIQ_LS_CR_SHIFT_IQSWAP)
#define AXIQ_LS_CR_FIELD_IQSWAP_OFF (0b0 << AXIQ_LS_CR_SHIFT_IQSWAP)
#define AXIQ_LS_CR_FIELD_CLRERR (0b1 << AXIQ_LS_CR_SHIFT_CLRERR)
#define AXIQ_LS_CR_FIELD_RESET (0b1 << AXIQ_LS_CR_SHIFT_RESET)
#define AXIQ_LS_CR_FIELD_COMPLEX (0b1 << AXIQ_LS_CR_SHIFT_COMPLEX)
#define AXIQ_LS_CR_FIELD_NONINT (0b1 << AXIQ_LS_CR_SHIFT_NONINT)

#define AXIQ_LS_CR_DISABLE (0b0 << AXIQ_LS_CR_SHIFT_ENABLE)
#define AXIQ_LS_CR_ENABLE (0b1 << AXIQ_LS_CR_SHIFT_ENABLE)
#define AXIQ_LS_CR_IQSWAP (0b1 << AXIQ_LS_CR_SHIFT_IQSWAP)
#define AXIQ_LS_CR_CLRERR (0b1 << AXIQ_LS_CR_SHIFT_CLRERR)
#define AXIQ_LS_CR_RESET (0b1 << AXIQ_LS_CR_SHIFT_RESET)

#define AXIQ_LS_RESET_MASK (AXIQ_LS_CR_FIELD_CLRERR | AXIQ_LS_CR_FIELD_RESET)
#define AXIQ_LS_ENABLE_MASK (AXIQ_LS_RESET_MASK | AXIQ_LS_CR_FIELD_ENABLE)

#define AXIQ_LS_CR_SINGLE_ANT_RESET_MASK(fifo) (((AXIQ_LS_RESET_MASK) << AXIQ_LS_CR_SHIFT(fifo)))

#define AXIQ_LS_CR_BOTH_ANT_RESET_MASK \
    (((AXIQ_LS_RESET_MASK) << AXIQ_LS_CR_SHIFT(0)) | ((AXIQ_LS_RESET_MASK) << AXIQ_LS_CR_SHIFT(1)))

#define AXIQ_LS_CR_SINGLE_ANT_EN_MASK(fifo) (((AXIQ_LS_ENABLE_MASK) << AXIQ_LS_CR_SHIFT(fifo)))

#define AXIQ_LS_CR_BOTH_ANT_EN_MASK \
    (((AXIQ_LS_ENABLE_MASK) << AXIQ_LS_CR_SHIFT(0)) | ((AXIQ_LS_ENABLE_MASK) << AXIQ_LS_CR_SHIFT(1)))

#define AXIQ_LS_CR_BOTH_ANT_EN_VAL \
    ((AXIQ_LS_CR_FIELD_ENABLE << AXIQ_LS_CR_SHIFT(0)) | (AXIQ_LS_CR_FIELD_ENABLE << AXIQ_LS_CR_SHIFT(1)))

#define AXIQ_LS_CR_BOTH_ANT_DIS_VAL \
    ((AXIQ_LS_CR_FIELD_DISABLE << AXIQ_LS_CR_SHIFT(0)) | (AXIQ_LS_CR_FIELD_DISABLE << AXIQ_LS_CR_SHIFT(1)))

#define AXIQ_LS_CR_BOTH_ANT_IQSWAP_MASK \
    (((AXIQ_LS_CR_FIELD_IQSWAP) << AXIQ_LS_CR_SHIFT(0)) | ((AXIQ_LS_CR_FIELD_IQSWAP) << AXIQ_LS_CR_SHIFT(1)))

#define AXIQ_LS_CR_BOTH_ANT_IQSWAP_OFF \
    (((AXIQ_LS_CR_FIELD_IQSWAP_OFF) << AXIQ_LS_CR_SHIFT(0)) | ((AXIQ_LS_CR_FIELD_IQSWAP_OFF) << AXIQ_LS_CR_SHIFT(1)))

#define AXIQ_LS_CR_BOTH_ANT_COMPLEX_MASK \
    ((AXIQ_LS_CR_FIELD_COMPLEX << AXIQ_LS_CR_SHIFT(0)) | (AXIQ_LS_CR_FIELD_COMPLEX << AXIQ_LS_CR_SHIFT(1)))

// =============================================================================
// Control register - RX Only Bits/Modes
// =============================================================================
#define AXIQ_LS_CR_RX_DECIM_MODE_B 7
#define AXIQ_LS_CR_RX_MAX_OUT_SEL_B 28
#define AXIQ_LS_CR_RX_MAX_RESTART_B 24

#define AXIQ_LS_CR_RX_DECIM_MODE_MASK (0b1 << AXIQ_LS_CR_RX_DECIM_MODE_B)
#define AXIQ_LS_CR_RX_MAX_RESTART_MASK (0b1 << AXIQ_LS_CR_RX_MAX_RESTART_B)
#define AXIQ_LS_CR_RX_MAX_OUT_SEL_MASK (0b111 << AXIQ_LS_CR_RX_MAX_OUT_SEL_B)
#define AXIQ_LS_CR_RX_MAX_MASK (AXIQ_LS_CR_RX_MAX_RESTART_MASK | AXIQ_LS_CR_RX_MAX_OUT_SEL_MASK)

#define AXIQ_LS_CR_RX_MAX_OUT_SEL_MAX (0b000)
#define AXIQ_LS_CR_RX_MAX_OUT_SEL_SUM0 (0b001)
#define AXIQ_LS_CR_RX_MAX_OUT_SEL_NUM0 (0b010)
#define AXIQ_LS_CR_RX_MAX_OUT_SEL_SUM1 (0b011)
#define AXIQ_LS_CR_RX_MAX_OUT_SEL_NUM1 (0b100)

#define AXIQ_LS_CR_RX_MAX_OUT_SEL_MAX_MASK ((AXIQ_LS_CR_RX_MAX_OUT_SEL_MAX) << (AXIQ_LS_CR_RX_MAX_OUT_SEL_B))
#define AXIQ_LS_CR_RX_MAX_OUT_SEL_SUM0_MASK ((AXIQ_LS_CR_RX_MAX_OUT_SEL_SUM0) << (AXIQ_LS_CR_RX_MAX_OUT_SEL_B))
#define AXIQ_LS_CR_RX_MAX_OUT_SEL_NUM0_MASK ((AXIQ_LS_CR_RX_MAX_OUT_SEL_NUM0) << (AXIQ_LS_CR_RX_MAX_OUT_SEL_B))
#define AXIQ_LS_CR_RX_MAX_OUT_SEL_SUM1_MASK ((AXIQ_LS_CR_RX_MAX_OUT_SEL_SUM1) << (AXIQ_LS_CR_RX_MAX_OUT_SEL_B))
#define AXIQ_LS_CR_RX_MAX_OUT_SEL_NUM1_MASK ((AXIQ_LS_CR_RX_MAX_OUT_SEL_NUM1) << (AXIQ_LS_CR_RX_MAX_OUT_SEL_B))
// =============================================================================
// Status register:
// =============================================================================

#define AXIQ_LS_SR_SHIFT(fifo) ((fifo)*8)

// RX FIFO status register (SR0) definitions:
#define AXIQ_LS_SR_SHIFT_RX0 0
#define AXIQ_LS_SR_SHIFT_RX1 8
#define AXIQ_LS_SR_SHIFT_RX2 16

#define AXIQ_LS_SR_FIELD_RX0 (0x7F << AXIQ_LS_SR_SHIFT_RX0)
#define AXIQ_LS_SR_FIELD_RX1 (0x7F << AXIQ_LS_SR_SHIFT_RX1)
#define AXIQ_LS_SR_FIELD_RX2 (0x7F << AXIQ_LS_SR_SHIFT_RX2)

// TX FIFO status register (SR1) definitions:
#define AXIQ_LS_SR_SHIFT_TX0 0
#define AXIQ_LS_SR_SHIFT_TX1 8

#define AXIQ_LS_SR_SHIFT_TX0 0
#define AXIQ_LS_SR_SHIFT_TX1 8

#define AXIQ_LS_SR_FIELD_TX0 (0x7F << AXIQ_LS_SR_SHIFT_TX0)
#define AXIQ_LS_SR_FIELD_TX1 (0x7F << AXIQ_LS_SR_SHIFT_TX1)

// FIFO status register definitions:
#define AXIQ_LS_SR_SHIFT_ENABLE 0
#define AXIQ_LS_SR_SHIFT_NOTEMPTY 1
#define AXIQ_LS_SR_SHIFT_ALLOWED 2
#define AXIQ_LS_SR_SHIFT_RESET 3
#define AXIQ_LS_SR_SHIFT_ERRUNDER 4
#define AXIQ_LS_SR_SHIFT_ERROVER 5
#define AXIQ_LS_SR_SHIFT_ERRRESET 6

#define AXIQ_LS_SR_FIELD_FIFO 0xFF
#define AXIQ_LS_SR_FIELD_ENABLE (0b1 << AXIQ_LS_SR_SHIFT_ENABLE)
#define AXIQ_LS_SR_FIELD_NOTEMPTY (0b1 << AXIQ_LS_SR_SHIFT_NOTEMPTY)
#define AXIQ_LS_SR_FIELD_ALLOWED (0b1 << AXIQ_LS_SR_SHIFT_ALLOWED)
#define AXIQ_LS_SR_FIELD_RESET (0b1 << AXIQ_LS_SR_SHIFT_RESET)
#define AXIQ_LS_SR_FIELD_ERRUNDER (0b1 << AXIQ_LS_SR_SHIFT_ERRUNDER)
#define AXIQ_LS_SR_FIELD_ERROVER (0b1 << AXIQ_LS_SR_SHIFT_ERROVER)
#define AXIQ_LS_SR_FIELD_ERRRESET (0b1 << AXIQ_LS_SR_SHIFT_ERRRESET)

#define AXIQ_LS_SR_BOTH_ANT_EN_MASK \
    (((AXIQ_LS_SR_FIELD_ENABLE) << AXIQ_LS_SR_SHIFT(0)) | ((AXIQ_LS_SR_FIELD_ENABLE) << AXIQ_LS_SR_SHIFT(1)))

#define AXIQ_LS_SR_BOTH_ANT_ALLOWED_MASK \
    (((AXIQ_LS_SR_FIELD_ALLOWED) << AXIQ_LS_SR_SHIFT(0)) | ((AXIQ_LS_SR_FIELD_ALLOWED) << AXIQ_LS_SR_SHIFT(1)))

// =============================================================================
// Physical driver interface:
// =============================================================================

// -----------------------------------------------------------------------------
// Bank-level access:
// -----------------------------------------------------------------------------

#pragma cplusplus on

//! @brief      Read an AXIQ bank control register.
//! @param      bank    specifies the AXIQ bank.
//! @return     The value of the AXIQ bank control register.
static inline uint32_t axiq_ls_bank_rx_cr(enum axiq_ls_bank_e bank) { return iord(AXIQ_LS_RX_CR(bank)); }

//! @brief      Write an AXIQ bank control register.
//! @param      bank    specifies the AXIQ bank.
//! @param      ctrl    the value to write.
//! @return     This function does not return a value.
static inline void axiq_ls_bank_rx_cr(enum axiq_ls_bank_e bank, uint32_t ctrl) { iowr(AXIQ_LS_RX_CR(bank), ctrl); }

//! @brief      Write an AXIQ bank control register field(s).
//! @param      bank    specifies the AXIQ bank.
//! @param      mask    specifies the fields to write.
//! @param      ctrl    the value to write.
//! @return     This function does not return a value.
static inline void axiq_ls_bank_rx_cr(enum axiq_ls_bank_e bank, uint32_t mask, uint32_t ctrl) {
    iowr(AXIQ_LS_RX_CR(bank), mask, ctrl);
}

//! @brief      Read an AXIQ bank control register.
//! @param      bank    specifies the AXIQ bank.
//! @return     The value of the AXIQ bank control register.
static inline uint32_t axiq_ls_bank_tx_cr(enum axiq_ls_bank_e bank) { return iord(AXIQ_LS_TX_CR(bank)); }

//! @brief      Write an AXIQ bank control register.
//! @param      bank    specifies the AXIQ bank.
//! @param      ctrl    the value to write.
//! @return     This function does not return a value.
static inline void axiq_ls_bank_tx_cr(enum axiq_ls_bank_e bank, uint32_t ctrl) { iowr(AXIQ_LS_TX_CR(bank), ctrl); }

//! @brief      Write an AXIQ bank control register field(s).
//! @param      bank    specifies the AXIQ bank.
//! @param      mask    specifies the fields to write.
//! @param      ctrl    the value to write.
//! @return     This function does not return a value.
static inline void axiq_ls_bank_tx_cr(enum axiq_ls_bank_e bank, uint32_t mask, uint32_t ctrl) {
    iowr(AXIQ_LS_TX_CR(bank), mask, ctrl);
}

//! @brief      Read an AXIQ bank status register.
//! @param      bank    specifies the AXIQ bank.
//! @return     The value of the AXIQ bank status register.
static inline uint32_t axiq_ls_bank_rx_sr(enum axiq_ls_bank_e bank) { return iord(AXIQ_LS_RX_SR(bank)); }

//! @brief      Read an AXIQ bank status register bit field.
//! @param      bank    specifies the AXIQ bank.
//! @param      mask    specifies the bit field mask.
//! @return     The value of the AXIQ bank status register bit field.
static inline uint32_t axiq_ls_bank_rx_sr(enum axiq_ls_bank_e bank, uint32_t mask) { return iord(AXIQ_LS_RX_SR(bank), mask); }

//! @brief      Read an AXIQ bank status register.
//! @param      bank    specifies the AXIQ bank.
//! @return     The value of the AXIQ bank status register.
static inline uint32_t axiq_ls_bank_tx_sr(enum axiq_ls_bank_e bank) { return iord(AXIQ_LS_TX_SR(bank)); }

//! @brief      Read an AXIQ bank status register bit field.
//! @param      bank    specifies the AXIQ bank.
//! @param      mask    specifies the bit field mask.
//! @return     The value of the AXIQ bank status register bit field.
static inline uint32_t axiq_ls_bank_tx_sr(enum axiq_ls_bank_e bank, uint32_t mask) { return iord(AXIQ_LS_TX_SR(bank), mask); }

#pragma cplusplus reset

//
// FIFO-level interface:
//
//! @brief      LS RX Shutdown one single path
//!             (abort/clear RX DMA activity, AXIQ FIFO ptr_rst, AXIQ RX disable)
//! @param      bank    specifies the LS-AXIQ bank.
//! @param      fifo    specifies the LS-AXIQ fifo.
//! @return     This function does not return a value.
//! @note       Call at end of RX window
extern void axiq_ls_rx_onepath_end(enum axiq_ls_bank_e bank, enum axiq_ls_fifo_rx_e fifo);

//! @brief      LS RX Shutdown both paths
//!             (abort/clear RX DMA activity, AXIQ FIFO ptr_rst, AXIQ RX disable)
//! @param      bank    specifies the LS-AXIQ bank.
//! @return     This function does not return a value.
//! @note       Call at end of RX window
extern void axiq_ls_rx_end(enum axiq_ls_bank_e bank);

//! @brief      Reset both RX FIFOs and enable RX_EN in an LS-AXIQ bank.
//! @param      bank    specifies the LS-AXIQ bank.
//! @return     This function does not return a value.
//! @note       Call before start of RX window
extern void axiq_ls_fifo_rx_reset(enum axiq_ls_bank_e bank);

//! @brief      Reset both TX FIFOs in an LS-AXIQ bank.
//! @param      bank    specifies the LS-AXIQ bank.
//! @return     This function does not return a value.
extern void axiq_ls_fifo_tx_reset(enum axiq_ls_bank_e bank);

//! @brief      Reset RX Max Search Window in an LS-AXIQ bank.
//!             Starts new RX search window and latches GPIN = num_rx_data
//! @param      bank    specifies the LS-AXIQ bank.
//! @return     This function does not return a value.
extern void axiq_ls_rx_max_reset(enum axiq_ls_bank_e bank);

//! @brief      Select "sum_rx_data" output to GPIN for RX0
//! @param      bank    specifies the LS-AXIQ bank.
//! @return     This function does not return a value.
extern void axiq_ls_rx0_max_out_sel_sum(enum axiq_ls_bank_e bank);

//! @brief      Select "num_rx_data" output to GPIN for RXO
//! @param      bank    specifies the LS-AXIQ bank.
//! @return     This function does not return a value.
extern void axiq_ls_rx0_max_out_sel_num(enum axiq_ls_bank_e bank);

//! @brief      Select "sum_rx_data" output to GPIN for RX1
//! @param      bank    specifies the LS-AXIQ bank.
//! @return     This function does not return a value.
extern void axiq_ls_rx1_max_out_sel_sum(enum axiq_ls_bank_e bank);

//! @brief      Select "num_rx_data" output to GPIN for RX1
//! @param      bank    specifies the LS-AXIQ bank.
//! @return     This function does not return a value.
extern void axiq_ls_rx1_max_out_sel_num(enum axiq_ls_bank_e bank);

// -----------------------------------------------------------------------------
// RX FIFO-level access:
// -----------------------------------------------------------------------------

#pragma cplusplus on

//! @brief      Read the RX FIFO control bits from the AXIQ control register.
//! @param      bank    specifies the AXIQ bank.
//! @param      fifo    specifies the RX FIFO.
//! @return     The in-place value of the RX FIFO control bits.
static inline uint32_t axiq_ls_fifo_rx_cr(enum axiq_ls_bank_e bank, enum axiq_ls_fifo_rx_e fifo) {
    return iord(AXIQ_LS_RX_CR(bank), AXIQ_LS_CR_FIELD_FIFO << AXIQ_LS_CR_SHIFT(fifo));
}

//! @brief      Write the RX FIFO control bits in the AXIQ control register.
//! @param      bank    specifies the AXIQ bank.
//! @param      fifo    specifies the RX FIFO.
//! @param      ctrl    the right-justified control value to write.
//! @return     This function does not return a value.
static inline void axiq_ls_fifo_rx_cr(enum axiq_ls_bank_e bank, enum axiq_ls_fifo_rx_e fifo, uint32_t ctrl) {
    iowr(AXIQ_LS_RX_CR(bank), AXIQ_LS_CR_FIELD_FIFO << AXIQ_LS_CR_SHIFT(fifo), ctrl << AXIQ_LS_CR_SHIFT(fifo));
}

//! @brief      Write the RX FIFO control bits in the AXIQ control register.
//! @param      bank    specifies the AXIQ bank.
//! @param      fifo    specifies the RX FIFO.
//! @param      mask    right-justified bit mask specifying the control bits to write.
//! @param      ctrl    the right-justified control value to write.
//! @return     This function does not return a value.
static inline void axiq_ls_fifo_rx_cr(enum axiq_ls_bank_e bank, enum axiq_ls_fifo_rx_e fifo, uint32_t mask, uint32_t ctrl) {
    iowr(AXIQ_LS_RX_CR(bank), (AXIQ_LS_CR_FIELD_FIFO & mask) << AXIQ_LS_CR_SHIFT(fifo), ctrl << AXIQ_LS_CR_SHIFT(fifo));
}

//! @brief      Read an AXIQ RX FIFO satus field.
//! @param      bank    specifies the AXIQ bank.
//! @param      fifo    specifies the AXIQ RX FIFO.
//! @return     The in-place value of the AXIQ RX FIFO status field.
static inline uint32_t axiq_ls_fifo_rx_sr(enum axiq_ls_bank_e bank, enum axiq_ls_fifo_rx_e fifo) {
    return iord(AXIQ_LS_RX_SR(bank), AXIQ_LS_SR_FIELD_FIFO << AXIQ_LS_CR_SHIFT(fifo));
}

//! @brief      Read an AXIQ RX FIFO status bit field.
//! @param      bank    specifies the AXIQ bank.
//! @param      fifo    specifies the AXIQ RX FIFO.
//! @param      mask    specifies the bit field mask.
//! @return     The in-place value of the AXIQ RX FIFO status bit field.
static inline uint32_t axiq_ls_fifo_rx_sr(enum axiq_ls_bank_e bank, enum axiq_ls_fifo_rx_e fifo, uint32_t mask) {
    return iord(AXIQ_LS_RX_SR(bank), (AXIQ_LS_SR_FIELD_FIFO & mask) << AXIQ_LS_CR_SHIFT(fifo));
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
static inline uint32_t axiq_ls_fifo_tx_cr(enum axiq_ls_bank_e bank, enum axiq_ls_fifo_tx_e fifo) {
    return iord(AXIQ_LS_TX_CR(bank), AXIQ_LS_CR_FIELD_FIFO << AXIQ_LS_CR_SHIFT(fifo));
}

//! @brief      Write the TX FIFO control bits in the AXIQ control register.
//! @param      bank    specifies the AXIQ bank.
//! @param      fifo    specifies the TX FIFO.
//! @param      ctrl    the right-justified control value to write.
//! @return     This function does not return a value.
static inline void axiq_ls_fifo_tx_cr(enum axiq_ls_bank_e bank, enum axiq_ls_fifo_tx_e fifo, uint32_t ctrl) {
    iowr(AXIQ_LS_TX_CR(bank), AXIQ_LS_CR_FIELD_FIFO << AXIQ_LS_CR_SHIFT(fifo), ctrl << AXIQ_LS_CR_SHIFT(fifo));
}

//! @brief      Write the TX FIFO control bits in the AXIQ control register.
//! @param      bank    specifies the AXIQ bank.
//! @param      fifo    specifies the TX FIFO.
//! @param      mask    right-justified bit mask specifying the control bits to write.
//! @param      ctrl    the right-justified control value to write.
//! @return     This function does not return a value.
static inline void axiq_ls_fifo_tx_cr(enum axiq_ls_bank_e bank, enum axiq_ls_fifo_tx_e fifo, uint32_t mask, uint32_t ctrl) {
    iowr(AXIQ_LS_TX_CR(bank), (AXIQ_LS_CR_FIELD_FIFO & mask) << AXIQ_LS_CR_SHIFT(fifo), ctrl << AXIQ_LS_CR_SHIFT(fifo));
}

//! @brief      Read an AXIQ TX FIFO satus field.
//! @param      bank    specifies the AXIQ bank.
//! @param      fifo    specifies the AXIQ TX FIFO.
//! @return     The in-place value of the AXIQ TX FIFO status field.
static inline uint32_t axiq_ls_fifo_tx_sr(enum axiq_ls_bank_e bank, enum axiq_ls_fifo_tx_e fifo) {
    return iord(AXIQ_LS_TX_SR(bank), AXIQ_LS_SR_FIELD_FIFO << AXIQ_LS_CR_SHIFT(fifo));
}

//! @brief      Read an AXIQ TX FIFO status bit field.
//! @param      bank    specifies the AXIQ bank.
//! @param      fifo    specifies the AXIQ TX FIFO.
//! @param      mask    specifies the bit field mask.
//! @return     The in-place value of the AXIQ TX FIFO status bit field.
static inline uint32_t axiq_ls_fifo_tx_sr(enum axiq_ls_bank_e bank, enum axiq_ls_fifo_tx_e fifo, uint32_t mask) {
    return iord(AXIQ_LS_TX_SR(bank), (AXIQ_LS_SR_FIELD_FIFO & mask) << AXIQ_LS_CR_SHIFT(fifo));
}

#pragma cplusplus reset

// =============================================================================
// Logical driver interface:
// =============================================================================

// -----------------------------------------------------------------------------
// Bank-level interface:
// -----------------------------------------------------------------------------

//! @brief      Disable all FIFOs in an AXIQ bank.
//! @param      bank    specifies the AXIQ bank.
//! @return     This function does not return a value.
static inline void axiq_ls_bank_disable(enum axiq_ls_bank_e bank) {
    // Disable RX FIFOs:
    axiq_ls_bank_rx_cr(bank,
                       (AXIQ_LS_CR_FIELD_ENABLE << AXIQ_LS_CR_SHIFT_RX0) | (AXIQ_LS_CR_FIELD_ENABLE << AXIQ_LS_CR_SHIFT_RX1) |
                           (AXIQ_LS_CR_FIELD_ENABLE << AXIQ_LS_CR_SHIFT_RX2),
                       (AXIQ_LS_CR_DISABLE << AXIQ_LS_CR_SHIFT_RX0) | (AXIQ_LS_CR_DISABLE << AXIQ_LS_CR_SHIFT_RX1) |
                           (AXIQ_LS_CR_DISABLE << AXIQ_LS_CR_SHIFT_RX2));

    // Disable TX FIFOs:
    axiq_ls_bank_tx_cr(bank, (AXIQ_LS_CR_FIELD_ENABLE << AXIQ_LS_CR_SHIFT_TX0) | (AXIQ_LS_CR_FIELD_ENABLE << AXIQ_LS_CR_SHIFT_TX1),
                       (AXIQ_LS_CR_DISABLE << AXIQ_LS_CR_SHIFT_TX0) | (AXIQ_LS_CR_DISABLE << AXIQ_LS_CR_SHIFT_TX1));
}

//! @brief      Enable all FIFOs in an AXIQ bank.
//! @param      bank    specifies the AXIQ bank.
//! @return     This function does not return a value.
static inline void axiq_ls_bank_enable(enum axiq_ls_bank_e bank) {
    // Enable RX FIFOs:
    axiq_ls_bank_rx_cr(bank,
                       (AXIQ_LS_CR_FIELD_ENABLE << AXIQ_LS_CR_SHIFT_RX0) | (AXIQ_LS_CR_FIELD_ENABLE << AXIQ_LS_CR_SHIFT_RX1) |
                           (AXIQ_LS_CR_FIELD_ENABLE << AXIQ_LS_CR_SHIFT_RX2),
                       (AXIQ_LS_CR_ENABLE << AXIQ_LS_CR_SHIFT_RX0) | (AXIQ_LS_CR_ENABLE << AXIQ_LS_CR_SHIFT_RX1) |
                           (AXIQ_LS_CR_ENABLE << AXIQ_LS_CR_SHIFT_RX2));

    // Enable TX FIFOs:
    axiq_ls_bank_tx_cr(bank, (AXIQ_LS_CR_FIELD_ENABLE << AXIQ_LS_CR_SHIFT_TX0) | (AXIQ_LS_CR_FIELD_ENABLE << AXIQ_LS_CR_SHIFT_TX1),
                       (AXIQ_LS_CR_ENABLE << AXIQ_LS_CR_SHIFT_TX0) | (AXIQ_LS_CR_ENABLE << AXIQ_LS_CR_SHIFT_TX1));
}

//! @brief      AXIQ bank status data structure.
struct axiq_ls_bank_status_t {
    uint32_t rx; //!< RX FIFOs status.
    uint32_t tx; //!< TX FIFOs status.
};

static inline struct axiq_ls_bank_status_t axiq_ls_bank_status(enum axiq_ls_bank_e bank) {
    struct axiq_ls_bank_status_t status;
    status.rx = axiq_ls_bank_rx_sr(bank);
    status.tx = axiq_ls_bank_tx_sr(bank);
    return status;
}

//! @brief      AXIQ bank error data structure.
struct axiq_ls_bank_error_t {
    uint32_t rx; //!< RX FIFOs errors.
    uint32_t tx; //!< TX FIFOs errors.
};

static inline struct axiq_ls_bank_error_t axiq_ls_bank_error(enum axiq_ls_bank_e bank) {
    struct axiq_ls_bank_error_t error;
    error.rx = axiq_ls_bank_rx_sr(
        bank, (AXIQ_LS_SR_FIELD_ERRUNDER << AXIQ_LS_SR_SHIFT_RX0) | (AXIQ_LS_SR_FIELD_ERROVER << AXIQ_LS_SR_SHIFT_RX0) |
                  (AXIQ_LS_SR_FIELD_ERRRESET << AXIQ_LS_SR_SHIFT_RX0) | (AXIQ_LS_SR_FIELD_ERRUNDER << AXIQ_LS_SR_SHIFT_RX1) |
                  (AXIQ_LS_SR_FIELD_ERROVER << AXIQ_LS_SR_SHIFT_RX1) | (AXIQ_LS_SR_FIELD_ERRRESET << AXIQ_LS_SR_SHIFT_RX1) |
                  (AXIQ_LS_SR_FIELD_ERRUNDER << AXIQ_LS_SR_SHIFT_RX2) | (AXIQ_LS_SR_FIELD_ERROVER << AXIQ_LS_SR_SHIFT_RX2) |
                  (AXIQ_LS_SR_FIELD_ERRRESET << AXIQ_LS_SR_SHIFT_RX2));
    error.tx = axiq_ls_bank_tx_sr(
        bank, (AXIQ_LS_SR_FIELD_ERRUNDER << AXIQ_LS_SR_SHIFT_TX0) | (AXIQ_LS_SR_FIELD_ERROVER << AXIQ_LS_SR_SHIFT_TX0) |
                  (AXIQ_LS_SR_FIELD_ERRRESET << AXIQ_LS_SR_SHIFT_TX0) | (AXIQ_LS_SR_FIELD_ERRUNDER << AXIQ_LS_SR_SHIFT_TX1) |
                  (AXIQ_LS_SR_FIELD_ERROVER << AXIQ_LS_SR_SHIFT_TX1) | (AXIQ_LS_SR_FIELD_ERRRESET << AXIQ_LS_SR_SHIFT_TX1));
    return error;
}

static inline void axiq_ls_bank_clrerr(enum axiq_ls_bank_e bank) {
    // Clear RX FIFOs errors:
    axiq_ls_bank_rx_cr(bank,
                       (AXIQ_LS_CR_FIELD_CLRERR << AXIQ_LS_CR_SHIFT_RX0) | (AXIQ_LS_CR_FIELD_CLRERR << AXIQ_LS_CR_SHIFT_RX1) |
                           (AXIQ_LS_CR_FIELD_CLRERR << AXIQ_LS_CR_SHIFT_RX2),
                       (AXIQ_LS_CR_CLRERR << AXIQ_LS_CR_SHIFT_RX0) | (AXIQ_LS_CR_CLRERR << AXIQ_LS_CR_SHIFT_RX1) |
                           (AXIQ_LS_CR_CLRERR << AXIQ_LS_CR_SHIFT_RX2));
    // Clear TX FIFOs errors:
    axiq_ls_bank_tx_cr(bank, (AXIQ_LS_CR_FIELD_CLRERR << AXIQ_LS_CR_SHIFT_TX0) | (AXIQ_LS_CR_FIELD_CLRERR << AXIQ_LS_CR_SHIFT_TX1),
                       (AXIQ_LS_CR_CLRERR << AXIQ_LS_CR_SHIFT_TX0) | (AXIQ_LS_CR_CLRERR << AXIQ_LS_CR_SHIFT_TX1));
}

// -----------------------------------------------------------------------------
// FIFO-level interface:
// -----------------------------------------------------------------------------

//! @brief      AXIQ FIFO descriptor.
struct axiq_ls_fifo_t {
    uint32_t addr; //!< FIFO address.
    uint32_t chan; //!< DMA channel number.
    uint32_t mask; //!< DMA channel bit mask.
};

//! @brief      Initialize an AXIQ RX FIFO descriptor.
//! @param      axiq    the AXIQ FIFO descriptor.
//! @param      bank    specifies the AXIQ bank.
//! @param      fifo    specifies the AXIQ FIFO.
//! @return     This function does not return a value.
static inline void axiq_ls_fifo_rx_open(struct axiq_ls_fifo_t *axiq, enum axiq_ls_bank_e bank, enum axiq_ls_fifo_rx_e fifo) {
    axiq->addr = AXIQ_LS_ADDR_RX(bank, fifo);
    axiq->chan = AXIQ_LS_CHAN_RX(bank, fifo);
    axiq->mask = AXIQ_LS_MASK_RX(bank, fifo);
}

//! @brief      Initialize an AXIQ TX FIFO descriptor.
//! @param      axiq    the AXIQ FIFO descriptor.
//! @param      bank    specifies the AXIQ bank.
//! @param      fifo    specifies the AXIQ FIFO.
//! @return     This function does not return a value.
static inline void axiq_ls_fifo_tx_open(struct axiq_ls_fifo_t *axiq, enum axiq_ls_bank_e bank, enum axiq_ls_fifo_tx_e fifo) {
    axiq->addr = AXIQ_LS_ADDR_TX(bank, fifo);
    axiq->chan = AXIQ_LS_CHAN_TX(bank, fifo);
    axiq->mask = AXIQ_LS_MASK_TX(bank, fifo);
}

static inline void axiq_ls_fifo_rx_disable(enum axiq_ls_bank_e bank, enum axiq_ls_fifo_rx_e fifo) {
    axiq_ls_fifo_rx_cr(bank, fifo, AXIQ_LS_CR_FIELD_ENABLE, AXIQ_LS_CR_DISABLE);
}

static inline void axiq_ls_fifo_tx_disable(enum axiq_ls_bank_e bank, enum axiq_ls_fifo_tx_e fifo) {
    axiq_ls_fifo_tx_cr(bank, fifo, AXIQ_LS_CR_FIELD_ENABLE, AXIQ_LS_CR_DISABLE);
}

static inline void axiq_ls_fifo_rx_enable(enum axiq_ls_bank_e bank, enum axiq_ls_fifo_rx_e fifo) {
    axiq_ls_fifo_rx_cr(bank, fifo, AXIQ_LS_CR_FIELD_ENABLE, AXIQ_LS_CR_ENABLE);
}

static inline void axiq_ls_fifo_tx_enable(enum axiq_ls_bank_e bank, enum axiq_ls_fifo_tx_e fifo) {
    axiq_ls_fifo_tx_cr(bank, fifo, AXIQ_LS_CR_FIELD_ENABLE, AXIQ_LS_CR_ENABLE);
}

static inline void axiq_ls_fifo_rx_clrerr(enum axiq_ls_bank_e bank, enum axiq_ls_fifo_rx_e fifo) {
    axiq_ls_fifo_rx_cr(bank, fifo, AXIQ_LS_CR_FIELD_CLRERR, AXIQ_LS_CR_CLRERR);
}

static inline void axiq_ls_fifo_tx_clrerr(enum axiq_ls_bank_e bank, enum axiq_ls_fifo_tx_e fifo) {
    axiq_ls_fifo_tx_cr(bank, fifo, AXIQ_LS_CR_FIELD_CLRERR, AXIQ_LS_CR_CLRERR);
}

static inline uint32_t axiq_ls_fifo_rx_status(enum axiq_ls_bank_e bank, enum axiq_ls_fifo_rx_e fifo) {
    return axiq_ls_fifo_rx_sr(bank, fifo);
}

static inline uint32_t axiq_ls_fifo_tx_status(enum axiq_ls_bank_e bank, enum axiq_ls_fifo_tx_e fifo) {
    return axiq_ls_fifo_tx_sr(bank, fifo);
}

static inline uint32_t axiq_ls_fifo_rx_error(enum axiq_ls_bank_e bank, enum axiq_ls_fifo_rx_e fifo) {
    return axiq_ls_fifo_rx_sr(bank, fifo, AXIQ_LS_SR_FIELD_ERRUNDER | AXIQ_LS_SR_FIELD_ERROVER | AXIQ_LS_SR_FIELD_ERRRESET);
}

static inline uint32_t axiq_ls_fifo_tx_error(enum axiq_ls_bank_e bank, enum axiq_ls_fifo_tx_e fifo) {
    return axiq_ls_fifo_tx_sr(bank, fifo, AXIQ_LS_SR_FIELD_ERRUNDER | AXIQ_LS_SR_FIELD_ERROVER | AXIQ_LS_SR_FIELD_ERRRESET);
}

#pragma cplusplus on

//! @brief      Write ONLY THE MASKED control bits in the AXIQ control register.
//! @param      bank    specifies the AXIQ bank.
//! @param      mask    right-justified bit mask specifying the control bits to write.
//! @param      ctrl    the right-justified control value to write.
//! @return     This function does not return a value.
static inline void axiq_ls_rx_wr(enum axiq_ls_bank_e bank, uint32_t mask, uint32_t ctrl) { iowr(AXIQ_LS_RX_CR(bank), mask, ctrl); }

//! @brief      Write a value to a LS RX AXIQ control register.
//! @param      bank    specifies the AXIQ bank.
//! @param      ctrl    the right-justified control value to write.
//! @return     This function does not return a value.
static inline void axiq_ls_rx_wr(enum axiq_ls_bank_e bank, uint32_t ctrl) { iowr(AXIQ_LS_RX_CR(bank), ctrl); }

//! @brief      Write a value to a LS TX AXIQ control register.
//! @param      bank    specifies the AXIQ bank.
//! @param      mask    right-justified bit mask specifying the control bits to write.
//! @param      ctrl    the right-justified control value to write.
//! @return     This function does not return a value.
static inline void axiq_ls_tx_wr(enum axiq_ls_bank_e bank, uint32_t mask, uint32_t ctrl) { iowr(AXIQ_LS_TX_CR(bank), mask, ctrl); }

//! @brief      LS AXIQ RX Reset (fifo_reset=1, clear_error=1, rx_en=0)
//! @param      bank:   specifies the AXIQ bank.
//! @param      fifo    specifies the AXIQ FIFO.
//! @return     This function does not return a value.
static inline void axiq_ls_rx_reset(enum axiq_ls_bank_e bank, enum axiq_ls_fifo_rx_e fifo) {
    axiq_ls_rx_wr(bank, AXIQ_LS_CR_SINGLE_ANT_EN_MASK(fifo), AXIQ_LS_CR_SINGLE_ANT_RESET_MASK(fifo));
}

//! @brief      LS AXIQ RX Reset (fifo_reset=1, clear_error=1, rx_en=0)
//! @param      bank:   specifies the AXIQ bank.
//! @return     This function does not return a value.
static inline void axiq_ls_rx_reset(enum axiq_ls_bank_e bank) {
    axiq_ls_rx_wr(bank, AXIQ_LS_CR_BOTH_ANT_EN_MASK, AXIQ_LS_CR_BOTH_ANT_RESET_MASK);
}

//! @brief      LS AXIQ RX Enable (rx_en=0).
//! @param      bank:   specifies the AXIQ bank.
//! @return     This function does not return a value.
static inline void axiq_ls_rx_disable(enum axiq_ls_bank_e bank) {
    axiq_ls_rx_wr(bank, AXIQ_LS_CR_BOTH_ANT_EN_VAL, AXIQ_LS_CR_BOTH_ANT_DIS_VAL);
}

//! @brief      LS AXIQ RX Enable (rx_en=1, fifo_reset=0, clear_error=0)
//! @param      bank:   specifies the AXIQ bank.
//! @return     This function does not return a value.
static inline void axiq_ls_rx_enable(enum axiq_ls_bank_e bank) {
    axiq_ls_rx_wr(bank, AXIQ_LS_CR_BOTH_ANT_EN_MASK, AXIQ_LS_CR_BOTH_ANT_EN_VAL);
}

//! @brief      LS AXIQ RX IQ Swap Enable (rx_swap=1)
//! @param      bank:   specifies the AXIQ bank.
//! @return     This function does not return a value.
static inline void axiq_ls_rx_iqswap_on(enum axiq_ls_bank_e bank) {
    axiq_ls_rx_wr(bank, AXIQ_LS_CR_BOTH_ANT_IQSWAP_MASK, AXIQ_LS_CR_BOTH_ANT_IQSWAP_MASK);
}

//! @brief      LS AXIQ RX IQ Swap Disable (rx_swap=0)
//! @param      bank:   specifies the AXIQ bank.
//! @return     This function does not return a value.
static inline void axiq_ls_rx_iqswap_off(enum axiq_ls_bank_e bank) {
    axiq_ls_rx_wr(bank, AXIQ_LS_CR_BOTH_ANT_IQSWAP_MASK, AXIQ_LS_CR_BOTH_ANT_IQSWAP_OFF);
}

//! @brief      LS AXIQ RX IQ Swap Configure
//! @param      bank:   specifies the AXIQ bank.
//! @param      flag:   value to write to RX IQSWAP bit: =0x0 to disable, =0x1 to enable
//! @return     This function does not return a value.
static inline void axiq_ls_rx_iqswap_config(enum axiq_ls_bank_e bank, uint32_t flag) {
    uint32_t val = ((flag << AXIQ_LS_CR_SHIFT(0)) | (flag << AXIQ_LS_CR_SHIFT(1))) << (AXIQ_LS_CR_SHIFT_IQSWAP);
    axiq_ls_rx_wr(bank, AXIQ_LS_CR_BOTH_ANT_IQSWAP_MASK, val);
}

//! @brief      LS AXIQ TX Reset (fifo_reset=1, clear_error=1)
//! @param      bank:   specifies the AXIQ bank.
//! @return     This function does not return a value.
static inline void axiq_ls_tx_reset(enum axiq_ls_bank_e bank) {
    axiq_ls_tx_wr(bank, AXIQ_LS_CR_BOTH_ANT_RESET_MASK, AXIQ_LS_CR_BOTH_ANT_RESET_MASK);
}

//! @brief      LS AXIQ TX Disable (tx_en=0).
//! @param      bank:   specifies the AXIQ bank.
//! @return     This function does not return a value.
static inline void axiq_ls_tx_disable(enum axiq_ls_bank_e bank) {
    axiq_ls_tx_wr(bank, AXIQ_LS_CR_BOTH_ANT_EN_VAL, AXIQ_LS_CR_BOTH_ANT_DIS_VAL);
}

//! @brief      LS AXIQ TX Enable (tx_en=1, fifo_reset=0, clear_error=0, complex=1)
//! @param      bank:   specifies the AXIQ bank.
//! @return     This function does not return a value.
static inline void axiq_ls_tx_enable(enum axiq_ls_bank_e bank) {
    axiq_ls_tx_wr(bank, AXIQ_LS_CR_BOTH_ANT_EN_MASK | AXIQ_LS_CR_BOTH_ANT_COMPLEX_MASK,
                  AXIQ_LS_CR_BOTH_ANT_EN_VAL | AXIQ_LS_CR_BOTH_ANT_COMPLEX_MASK);
}

//! @brief      LS AXIQ TX IQ Swap Configure
//! @param      bank:   specifies the AXIQ bank.
//! @param      flag:   value to write to TX IQSWAP bit: =0x0 to disable, =0x1 to enable
//! @return     This function does not return a value.
static inline void axiq_ls_tx_iqswap_config(enum axiq_ls_bank_e bank, uint32_t flag) {
    uint32_t val = ((flag << AXIQ_LS_CR_SHIFT(0)) | (flag << AXIQ_LS_CR_SHIFT(1))) << (AXIQ_LS_CR_SHIFT_IQSWAP);
    axiq_ls_tx_wr(bank, AXIQ_LS_CR_BOTH_ANT_IQSWAP_MASK, val);
}

#define AXIQ_LS_FIFO_RD (DMAC_RDC | DMAC_FIFO)
#define AXIQ_LS_FIFO_WR (DMAC_WRC | DMAC_FIFO)

static inline void axiq_ls_fifo_rx_read(enum axiq_ls_bank_e bank, enum axiq_ls_fifo_rx_e fifo, uint32_t ctrl, void *data,
                                        size_t size) {
    dmac_enable(ctrl | AXIQ_LS_FIFO_RD | AXIQ_LS_CHAN_RX(bank, fifo), size, AXIQ_LS_ADDR_RX(bank, fifo), data);
}

static inline void axiq_ls_fifo_rx_read(enum axiq_ls_bank_e bank, enum axiq_ls_fifo_rx_e fifo, void *data, size_t size) {
    dmac_enable(DMAC_TRIG_VCPU | AXIQ_LS_FIFO_RD | AXIQ_LS_CHAN_RX(bank, fifo), size, AXIQ_LS_ADDR_RX(bank, fifo), data);
}

static inline void axiq_ls_fifo_rx_read(struct axiq_ls_fifo_t *axiq, uint32_t ctrl, void *data, size_t size) {
    dmac_enable(ctrl | AXIQ_LS_FIFO_RD | axiq->chan, size, axiq->addr, data);
}

static inline void axiq_ls_fifo_tx_write(enum axiq_ls_bank_e bank, enum axiq_ls_fifo_tx_e fifo, uint32_t ctrl, void const *data,
                                         size_t size) {
    dmac_enable(ctrl | AXIQ_LS_FIFO_WR | AXIQ_LS_CHAN_TX(bank, fifo), size, AXIQ_LS_ADDR_TX(bank, fifo), (void *)data);
}

static inline void axiq_ls_fifo_tx_write(enum axiq_ls_bank_e bank, enum axiq_ls_fifo_tx_e fifo, void const *data, size_t size) {
    dmac_enable(DMAC_TRIG_VCPU | AXIQ_LS_FIFO_WR | AXIQ_LS_CHAN_TX(bank, fifo), size, AXIQ_LS_ADDR_TX(bank, fifo), (void *)data);
}

static inline void axiq_ls_fifo_tx_write(struct axiq_ls_fifo_t *axiq, uint32_t ctrl, void const *data, size_t size) {
    dmac_enable(ctrl | AXIQ_LS_FIFO_WR | axiq->chan, size, axiq->addr, (void *)data);
}

#define AXIQ_FIFO_LS_RD (DMAC_RDC | DMAC_FIFO)
#define AXIQ_FIFO_LS_WR (DMAC_WRC | DMAC_FIFO)

static inline void axiq_ls_fifo_read(enum axiq_ls_bank_e bank, enum axiq_ls_fifo_rx_e fifo, uint32_t ctrl, void *data,
                                     size_t size) {
    dmac_enable(ctrl | AXIQ_FIFO_LS_RD | AXIQ_LS_CHAN_RX(bank, fifo), size, AXIQ_LS_ADDR_RX(bank, fifo), data);
}

static inline void axiq_ls_fifo_read(enum axiq_ls_bank_e bank, enum axiq_ls_fifo_rx_e fifo, void *data, size_t size) {
    dmac_enable(AXIQ_FIFO_LS_RD | AXIQ_LS_CHAN_RX(bank, fifo), size, AXIQ_LS_ADDR_RX(bank, fifo), data);
}

static inline void axiq_ls_fifo_read(struct axiq_ls_fifo_t *axiq, uint32_t ctrl, void *data, size_t size) {
    dmac_enable(ctrl | AXIQ_FIFO_LS_RD | axiq->chan, size, axiq->addr, data);
}

//! @brief      Read an RX AXIQ bank's RX_ALLOWED bit mask
//! @param      bank    specifies the AXIQ bank.
//! @return     The RX_ALLOWED bit mask value.
static inline uint32_t axiq_ls_is_rx_allowed(enum axiq_ls_bank_e bank) {
    return iord(AXIQ_LS_RX_SR(bank), AXIQ_LS_SR_BOTH_ANT_ALLOWED_MASK);
}

//! @brief      Read an TX AXIQ bank's TX_ALLOWED bit mask
//! @param      bank    specifies the AXIQ bank.
//! @return     The TX_ALLOWED bit mask value.
static inline uint32_t axiq_ls_is_tx_allowed(enum axiq_ls_bank_e bank) {
    return iord(AXIQ_LS_TX_SR(bank), AXIQ_LS_SR_BOTH_ANT_ALLOWED_MASK);
}

//! @brief      Read an TX AXIQ bank's TX_EN bit mask
//! @param      bank    specifies the AXIQ bank.
//! @return     The TX_EN bit mask value.
static inline uint32_t axiq_ls_is_tx_enabled(enum axiq_ls_bank_e bank) {
    return iord(AXIQ_LS_TX_SR(bank), AXIQ_LS_SR_BOTH_ANT_EN_MASK);
}

#pragma cplusplus reset

//
// FIFO-level interface:
//

//! @brief      LS RX Shutdown (abort/clear RX DMA activity, AXIQ FIFO ptr_rst, AXIQ RX disable)
//! @param      bank    specifies the LS-AXIQ bank.
//! @return     This function does not return a value.
//! @note       Call at end of RX window
extern void axiq_ls_rx_end(enum axiq_ls_bank_e bank);

//! @brief      Reset RX FIFO and enable RX_EN in an LS-AXIQ bank.
//! @param      bank    specifies the LS-AXIQ bank.
//! @return     This function does not return a value.
//! @note       Call before start of RX window
extern void axiq_ls_fifo_rx_reset(enum axiq_ls_bank_e bank);

#endif // __AXIQ_LS_H__
