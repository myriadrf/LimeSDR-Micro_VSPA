// SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
// Copyright 2019 - 2025   NXP Semiconductors

// =============================================================================
//! @file       axiq-hs.h
//! @brief      High-speed AXIQ driver.
//! @author     NXP Semiconductors.
//!
//! @details    This file provides a software interface for the High-Speed
//!             AXI I/Q interface AXIQ_HS. The hardware includes a single
//!             AXIQ_HS module that is, from a software perspective, perceived
//!             as 2 banks (@c AXIQ_HS_BANK_0 and @c AXIQ_HS_BANK_1).
//!             Each bank provides 1 receive channel (AXIQ_HS_FIFO_RX)
//!             and 1 transmit channel (AXIQ_HS_FIFO_TX).
//!
// =============================================================================

#ifndef __AXIQ_HS_H__
#define __AXIQ_HS_H__

// =============================================================================
// AXIQ FIFOs:
// =============================================================================

//! @brief      AXIQ FIFO enumeration.
enum axiq_hs_fifo_e {
    AXIQ_HS_FIFO_RX,    //!< AXIQ FIFO RX.
    AXIQ_HS_FIFO_TX,    //!< AXIQ FIFO TX.
    AXIQ_HS_FIFO_RX_CNT //!< Total number of FIFOs.
};

// =============================================================================
// Control register:
// =============================================================================
#define AXIQ_HS_CR_TX_SHIFT 16

//! @brief      Number of right-shifts to right-justify the FIFO control bits.
#define AXIQ_HS_CR_SHIFT(bank, fifo) ((fifo)*16)

#define AXIQ_HS_CR_SHIFT_RX(bank, fifo) AXIQ_HS_CR_SHIFT(bank, fifo)
#define AXIQ_HS_CR_SHIFT_TX(bank, fifo) AXIQ_HS_CR_SHIFT(bank, fifo)

#define AXIQ_HS_CR_FIELD_RX(bank, fifo) (0x3F << AXIQ_HS_CR_SHIFT_RX(bank, fifo))
#define AXIQ_HS_CR_FIELD_TX(bank, fifo) (0x3F << AXIQ_HS_CR_SHIFT_TX(bank, fifo))

#define AXIQ_HS_CR_SHIFT_ENABLE 0
#define AXIQ_HS_CR_SHIFT_IQSWAP 3
#define AXIQ_HS_CR_SHIFT_CLRERR 4
#define AXIQ_HS_CR_SHIFT_RESET 5

#define AXIQ_HS_CR_FIELD_ENABLE (0b1 << AXIQ_HS_CR_SHIFT_ENABLE)
#define AXIQ_HS_CR_FIELD_DISABLE (0b0 << AXIQ_HS_CR_SHIFT_ENABLE)
#define AXIQ_HS_CR_FIELD_IQSWAP (0b1 << AXIQ_HS_CR_SHIFT_IQSWAP)
#define AXIQ_HS_CR_FIELD_IQSWAP_OFF (0b0 << AXIQ_HS_CR_SHIFT_IQSWAP)
#define AXIQ_HS_CR_FIELD_CLRERR (0b1 << AXIQ_HS_CR_SHIFT_CLRERR)
#define AXIQ_HS_CR_FIELD_RESET (0b1 << AXIQ_HS_CR_SHIFT_RESET)

#define AXIQ_HS_RESET_MASK (AXIQ_HS_CR_FIELD_CLRERR | AXIQ_HS_CR_FIELD_RESET)
#define AXIQ_HS_ENABLE_MASK (AXIQ_HS_RESET_MASK | AXIQ_HS_CR_FIELD_ENABLE)

#define AXIQ_HS_CR_FIELD_ENABLE_ALL                                                   \
    ((AXIQ_HS_CR_FIELD_ENABLE << AXIQ_HS_CR_SHIFT(AXIQ_HS_BANK_0, AXIQ_HS_FIFO_RX)) | \
     (AXIQ_HS_CR_FIELD_ENABLE << AXIQ_HS_CR_SHIFT(AXIQ_HS_BANK_0, AXIQ_HS_FIFO_TX)))

#define AXIQ_HS_CR_FIELD_DISABLE_ALL                                                   \
    ((AXIQ_HS_CR_FIELD_DISABLE << AXIQ_HS_CR_SHIFT(AXIQ_HS_BANK_0, AXIQ_HS_FIFO_RX)) | \
     (AXIQ_HS_CR_FIELD_DISABLE << AXIQ_HS_CR_SHIFT(AXIQ_HS_BANK_0, AXIQ_HS_FIFO_TX)))

// =============================================================================
// Control register - RX Only Bits/Modes
// =============================================================================
#define AXIQ_HS_CR_RX_DECIM_MODE_B 6
#define AXIQ_HS_CR_RX_MAX_OUT_SEL_B 29
#define AXIQ_HS_CR_RX_MAX_RESTART_B 31

#define AXIQ_HS_CR_RX_DECIM_MODE_MASK (0b1 << AXIQ_HS_CR_RX_DECIM_MODE_B)
#define AXIQ_HS_CR_RX_MAX_RESTART_MASK (0b1 << AXIQ_HS_CR_RX_MAX_RESTART_B)
#define AXIQ_HS_CR_RX_MAX_OUT_SEL_MASK (0b11 << AXIQ_HS_CR_RX_MAX_OUT_SEL_B)
#define AXIQ_HS_CR_RX_MAX_MASK (AXIQ_HS_CR_RX_MAX_RESTART_MASK | AXIQ_HS_CR_RX_MAX_OUT_SEL_MASK)

#define AXIQ_HS_CR_RX_MAX_OUT_SEL_MAX (0b00)
#define AXIQ_HS_CR_RX_MAX_OUT_SEL_SUM (0b10)
#define AXIQ_HS_CR_RX_MAX_OUT_SEL_NUM (0b11)

#define AXIQ_HS_CR_RX_MAX_OUT_SEL_MAX_MASK ((AXIQ_HS_CR_RX_MAX_OUT_SEL_MAX) << (AXIQ_HS_CR_RX_MAX_OUT_SEL_B))
#define AXIQ_HS_CR_RX_MAX_OUT_SEL_SUM_MASK ((AXIQ_HS_CR_RX_MAX_OUT_SEL_SUM) << (AXIQ_HS_CR_RX_MAX_OUT_SEL_B))
#define AXIQ_HS_CR_RX_MAX_OUT_SEL_NUM_MASK ((AXIQ_HS_CR_RX_MAX_OUT_SEL_NUM) << (AXIQ_HS_CR_RX_MAX_OUT_SEL_B))

// =============================================================================
// Status register:
// =============================================================================

#define AXIQ_HS_SR_SHIFT(bank, fifo) (((bank)*16) + ((fifo)*8))

#define AXIQ_HS_SR_SHIFT_RX(bank, fifo) AXIQ_HS_SR_SHIFT(bank, AXIQ_HS_FIFO_RX)
#define AXIQ_HS_SR_SHIFT_TX(bank, fifo) AXIQ_HS_SR_SHIFT(bank, AXIQ_HS_FIFO_TX)

#define AXIQ_HS_SR_FIELD_RX(bank, fifo) (0b1 << AXIQ_HS_SR_SHIFT_RX(bank, fifo))
#define AXIQ_HS_SR_FIELD_TX(bank, fifo) (0b1 << AXIQ_HS_SR_SHIFT_TX(bank, fifo))

#define AXIQ_HS_SR_SHIFT_ENABLE 0
#define AXIQ_HS_SR_SHIFT_NOTEMPTY 1
#define AXIQ_HS_SR_SHIFT_ALLOWED 2
#define AXIQ_HS_SR_SHIFT_RESET 3
#define AXIQ_HS_SR_SHIFT_ERRUNDER 4
#define AXIQ_HS_SR_SHIFT_ERROVER 5
#define AXIQ_HS_SR_SHIFT_ERRRESET 6

#define AXIQ_HS_SR_FIELD_ENABLE (0b1 << AXIQ_HS_SR_SHIFT_ENABLE)
#define AXIQ_HS_SR_FIELD_NOTEMPTY (0b1 << AXIQ_HS_SR_SHIFT_NOTEMPTY)
#define AXIQ_HS_SR_FIELD_ALLOWED (0b1 << AXIQ_HS_SR_SHIFT_ALLOWED)
#define AXIQ_HS_SR_FIELD_RESET (0b1 << AXIQ_HS_SR_SHIFT_RESET)
#define AXIQ_HS_SR_FIELD_ERRUNDER (0b1 << AXIQ_HS_SR_SHIFT_ERRUNDER)
#define AXIQ_HS_SR_FIELD_ERROVER (0b1 << AXIQ_HS_SR_SHIFT_ERROVER)
#define AXIQ_HS_SR_FIELD_ERRRESET (0b1 << AXIQ_HS_SR_SHIFT_ERRRESET)

// =============================================================================
// Physical driver interface:
// =============================================================================

#pragma cplusplus on

//! @brief      Read an AXIQ bank control register.
//! @param      bank    specifies the AXIQ bank.
//! @return     The value of the AXIQ bank control register.
static inline uint32_t axiq_hs_bank_rx_cr(enum axiq_hs_bank_e bank) {
    return iord(AXIQ_HS_RX_CR(bank), AXIQ_HS_CR_FIELD_RX(bank, AXIQ_HS_FIFO_RX));
}

//! @brief      Write an AXIQ bank control register.
//! @param      bank    specifies the AXIQ bank.
//! @param      ctrl    the value to write.
//! @return     This function does not return a value.
static inline void axiq_hs_bank_rx_cr(enum axiq_hs_bank_e bank, uint32_t ctrl) {
    iowr(AXIQ_HS_RX_CR(bank), AXIQ_HS_CR_FIELD_RX(bank, AXIQ_HS_FIFO_RX), ctrl);
}

//! @brief      Write an AXIQ bank control register field(s).
//! @param      bank    specifies the AXIQ bank.
//! @param      mask    specifies the fields to write.
//! @param      ctrl    the value to write.
//! @return     This function does not return a value.
static inline void axiq_hs_bank_rx_cr(enum axiq_hs_bank_e bank, uint32_t mask, uint32_t ctrl) {
    iowr(AXIQ_HS_RX_CR(bank), mask, ctrl);
}

//! @brief      Read an AXIQ bank control register.
//! @param      bank    specifies the AXIQ bank.
//! @return     The value of the AXIQ bank control register.
static inline uint32_t axiq_hs_bank_tx_cr(enum axiq_hs_bank_e bank) {
    return iord(AXIQ_HS_TX_CR(bank), AXIQ_HS_CR_FIELD_TX(bank, AXIQ_HS_FIFO_TX));
}

//! @brief      Write an AXIQ bank control register.
//! @param      bank    specifies the AXIQ bank.
//! @param      ctrl    the value to write.
//! @return     This function does not return a value.
static inline void axiq_hs_bank_tx_cr(enum axiq_hs_bank_e bank, uint32_t ctrl) {
    iowr(AXIQ_HS_TX_CR(bank), AXIQ_HS_CR_FIELD_TX(bank, AXIQ_HS_FIFO_TX), ctrl);
}

//! @brief      Write an AXIQ bank control register field(s).
//! @param      bank    specifies the AXIQ bank.
//! @param      mask    specifies the fields to write.
//! @param      ctrl    the value to write.
//! @return     This function does not return a value.
static inline void axiq_hs_bank_tx_cr(enum axiq_hs_bank_e bank, uint32_t mask, uint32_t ctrl) {
    iowr(AXIQ_HS_TX_CR(bank), mask, ctrl);
}

//! @brief      Read an AXIQ bank status register.
//! @param      bank    specifies the AXIQ bank.
//! @return     The value of the AXIQ bank status register.
static inline uint32_t axiq_hs_bank_rx_sr(enum axiq_hs_bank_e bank) {
    return iord(AXIQ_HS_RX_SR(bank), AXIQ_HS_SR_FIELD_RX(bank, AXIQ_HS_FIFO_RX));
}

//! @brief      Read an AXIQ bank status register bit field.
//! @param      bank    specifies the AXIQ bank.
//! @param      mask    specifies the bit field mask.
//! @return     The value of the AXIQ bank status register bit field.
static inline uint32_t axiq_hs_bank_rx_sr(enum axiq_hs_bank_e bank, uint32_t mask) { return iord(AXIQ_HS_RX_SR(bank), mask); }

//! @brief      Read an AXIQ bank status register.
//! @param      bank    specifies the AXIQ bank.
//! @return     The value of the AXIQ bank status register.
static inline uint32_t axiq_hs_bank_tx_sr(enum axiq_hs_bank_e bank) {
    return iord(AXIQ_HS_TX_SR(bank), AXIQ_HS_SR_FIELD_RX(bank, AXIQ_HS_FIFO_RX));
}

//! @brief      Read an AXIQ bank status register bit field.
//! @param      bank    specifies the AXIQ bank.
//! @param      mask    specifies the bit field mask.
//! @return     The value of the AXIQ bank status register bit field.
static inline uint32_t axiq_hs_bank_tx_sr(enum axiq_hs_bank_e bank, uint32_t mask) { return iord(AXIQ_HS_TX_SR(bank), mask); }

//! @brief      Read an RX AXIQ bank's RX_ALLOWED bit mask
//! @param      bank    specifies the AXIQ bank.
//! @return     The RX_ALLOWED bit mask value.
static inline uint32_t axiq_hs_is_rx_allowed(enum axiq_hs_bank_e bank) {
    return iord(AXIQ_HS_RX_SR(bank), AXIQ_HS_SR_FIELD_ALLOWED);
}

#pragma cplusplus reset

//
// Bank-level interface:
//

//! @brief      Reset an HS-AXIQ bank.
//! @param      bank    specifies the AXIQ bank.
//! @return     This function does not return a value.
extern void axiq_hs_bank_reset(enum axiq_hs_bank_e bank);

//
// FIFO-level interface:
//

//! @brief      HS RX Shutdown (abort/clear RX DMA activity, AXIQ FIFO ptr_rst, AXIQ RX disable)
//! @param      bank    specifies the HS-AXIQ bank.
//! @return     This function does not return a value.
//! @note       Call at end of RX window
extern void axiq_hs_rx_end(enum axiq_hs_bank_e bank);

//! @brief      Reset RX FIFO and enable RX_EN in an HS-AXIQ bank.
//! @param      bank    specifies the HS-AXIQ bank.
//! @return     This function does not return a value.
//! @note       Call before start of RX window
extern void axiq_hs_fifo_rx_reset(enum axiq_hs_bank_e bank);

//! @brief      Reset TX FIFO in an HS-AXIQ bank.
//! @param      bank    specifies the HS-AXIQ bank.
//! @return     This function does not return a value.
extern void axiq_hs_fifo_tx_reset(enum axiq_hs_bank_e bank);

//! @brief      Reset RX Max Search Window in an HS-AXIQ bank.
//!             Starts new RX search window and latches GPIN = num_rx_data
//! @param      bank    specifies the HS-AXIQ bank.
//! @return     This function does not return a value.
extern void axiq_hs_rx_max_reset(enum axiq_hs_bank_e bank);

//! @brief      Select "sum_rx_data" output to GPIN
//! @param      bank    specifies the HS-AXIQ bank.
//! @return     This function does not return a value.
extern void axiq_hs_rx_max_out_sel_sum(enum axiq_hs_bank_e bank);

//! @brief      Select "num_rx_data" output to GPIN
//! @param      bank    specifies the HS-AXIQ bank.
//! @return     This function does not return a value.
extern void axiq_hs_rx_max_out_sel_num(enum axiq_hs_bank_e bank);

// -----------------------------------------------------------------------------
// RX FIFO-level access:
// -----------------------------------------------------------------------------

#pragma cplusplus on

//! @brief      Read the RX FIFO control bits from the AXIQ control register.
//! @param      bank    specifies the AXIQ bank.
//! @param      fifo    specifies the RX FIFO.
//! @return     The in-place value of the RX FIFO control bits.
static inline uint32_t axiq_hs_fifo_rx_cr(enum axiq_hs_bank_e bank, enum axiq_hs_fifo_e fifo) {
    return iord(AXIQ_HS_RX_CR(bank), AXIQ_HS_CR_FIELD_RX(bank, fifo));
}

//! @brief      Write the RX FIFO control bits in the AXIQ control register.
//! @param      bank    specifies the AXIQ bank.
//! @param      fifo    specifies the RX FIFO.
//! @param      ctrl    the right-justified control value to write.
//! @return     This function does not return a value.
static inline void axiq_hs_fifo_rx_cr(enum axiq_hs_bank_e bank, enum axiq_hs_fifo_e fifo, uint32_t ctrl) {
    iowr(AXIQ_HS_RX_CR(bank), AXIQ_HS_CR_FIELD_RX(bank, fifo), ctrl << AXIQ_HS_CR_SHIFT(bank, fifo));
}

//! @brief      Write the RX FIFO control bits in the AXIQ control register.
//! @param      bank    specifies the AXIQ bank.
//! @param      fifo    specifies the RX FIFO.
//! @param      mask    right-justified bit mask specifying the control bits to write.
//! @param      ctrl    the right-justified control value to write.
//! @return     This function does not return a value.
static inline void axiq_hs_fifo_rx_cr(enum axiq_hs_bank_e bank, enum axiq_hs_fifo_e fifo, uint32_t mask, uint32_t ctrl) {
    iowr(AXIQ_HS_RX_CR(bank), (AXIQ_HS_CR_FIELD_RX(bank, fifo) & mask), ctrl << AXIQ_HS_CR_SHIFT(bank, fifo));
}

//! @brief      Read an AXIQ RX FIFO status field.
//! @param      bank    specifies the AXIQ bank.
//! @param      fifo    specifies the AXIQ RX FIFO.
//! @return     The in-place value of the AXIQ RX FIFO status field.
static inline uint32_t axiq_hs_fifo_rx_sr(enum axiq_hs_bank_e bank, enum axiq_hs_fifo_e fifo) {
    return iord(AXIQ_HS_RX_SR(bank), AXIQ_HS_SR_FIELD_RX(bank, fifo));
}

//! @brief      Read an AXIQ RX FIFO status bit field.
//! @param      bank    specifies the AXIQ bank.
//! @param      fifo    specifies the AXIQ RX FIFO.
//! @param      mask    specifies the bit field mask.
//! @return     The in-place value of the AXIQ RX FIFO status bit field.
static inline uint32_t axiq_hs_fifo_rx_sr(enum axiq_hs_bank_e bank, enum axiq_hs_fifo_e fifo, uint32_t mask) {
    return iord(AXIQ_HS_RX_SR(bank), (AXIQ_HS_SR_FIELD_RX(bank, fifo) & mask));
}

#pragma cplusplus reset

//! @brief      AXIQ FIFO descriptor.
struct axiq_hs_fifo_t {
    uint32_t addr; //!< FIFO address.
    uint32_t chan; //!< DMA channel number.
    uint32_t mask; //!< DMA channel bit mask.
};

#define AXIQ_FIFO_RD (DMAC_RDC | DMAC_MBRE | DMAC_FIFO)
#if 1 // TBD TECH: AXIQ reset between ALLOWED windows works better w/ EXT_TRIG=1 //defined(__CAS__)
#define AXIQ_FIFO_HS_RD (DMAC_RDC | DMAC_FIFO)
#else
#define AXIQ_FIFO_HS_RD (DMAC_RDC | DMAC_FIFO_HS)
#endif
#define AXIQ_FIFO_WR (DMAC_WRC | DMAC_FIFO)
#define AXIQ_FIFO_HS_WR (DMAC_WRC | DMAC_FIFO_HS)

//! @brief      Initialize an AXIQ FIFO descriptor.
//! @param      axiq
//! @param      bank    specifies the AXIQ bank.
//! @param      fifo    specifies the AXIQ FIFO.
//! @return     This function does not return a value.
static inline void axiq_hs_fifo_open(struct axiq_hs_fifo_t *axiq, enum axiq_hs_bank_e bank, enum axiq_hs_fifo_e fifo) {
    axiq->addr = AXIQ_HS_ADDR_RX(bank, fifo);
    axiq->chan = AXIQ_HS_CHAN_RX(bank, fifo);
    axiq->mask = AXIQ_HS_MASK_RX(bank, fifo);
}

static inline void axiq_hs_fifo_enable(enum axiq_hs_bank_e bank, enum axiq_hs_fifo_e fifo) {
    axiq_hs_fifo_rx_cr(bank, fifo, AXIQ_HS_CR_FIELD_ENABLE);
}

static inline void axiq_hs_fifo_disable(enum axiq_hs_bank_e bank, enum axiq_hs_fifo_e fifo) {
    axiq_hs_fifo_rx_cr(bank, fifo, AXIQ_HS_CR_FIELD_DISABLE);
}

static inline void axiq_hs_fifo_iqswap_on(enum axiq_hs_bank_e bank, enum axiq_hs_fifo_e fifo) {
    axiq_hs_fifo_rx_cr(bank, fifo, AXIQ_HS_CR_FIELD_IQSWAP);
}

static inline void axiq_hs_fifo_iqswap_off(enum axiq_hs_bank_e bank, enum axiq_hs_fifo_e fifo) {
    axiq_hs_fifo_rx_cr(bank, fifo, AXIQ_HS_CR_FIELD_IQSWAP_OFF);
}

// This function sets RESET, CLR_ERR bits
static inline void axiq_hs_fifo_reset(enum axiq_hs_bank_e bank, enum axiq_hs_fifo_e fifo) {
    axiq_hs_fifo_rx_cr(bank, fifo, AXIQ_HS_RESET_MASK);
}

//! @brief      Write ONLY THE MASKED control bits in the AXIQ control register.
//! @param      bank    specifies the AXIQ bank.
//! @param      mask    right-justified bit mask specifying the control bits to write.
//! @param      ctrl    the right-justified control value to write.
//! @return     This function does not return a value.
static inline void axiq_hs_rx_wr(enum axiq_hs_bank_e bank, uint32_t mask, uint32_t ctrl) { iowr(AXIQ_HS_RX_CR(bank), mask, ctrl); }

//! @brief      Write a value to the AXIQ control register.
//! @param      bank    specifies the AXIQ bank.
//! @param      ctrl    the right-justified control value to write.
//! @return     This function does not return a value.
static inline void axiq_hs_rx_wr_val(enum axiq_hs_bank_e bank, uint32_t ctrl) { iowr(AXIQ_HS_RX_CR(bank), ctrl); }

//! @brief      HS AXIQ RX Enable (rx_en=0).
//! @param      bank:   specifies the AXIQ bank.
//! @return     This function does not return a value.
static inline void axiq_hs_rx_disable(enum axiq_hs_bank_e bank) {
    axiq_hs_rx_wr(bank, AXIQ_HS_CR_FIELD_ENABLE, AXIQ_HS_CR_FIELD_DISABLE);
}

//! @brief      HS AXIQ RX Reset (fifo_reset=1, clear_error=1, rx_en0)
//! @param      bank:   specifies the AXIQ bank.
//! @return     This function does not return a value.
static inline void axiq_hs_rx_reset(enum axiq_hs_bank_e bank) { axiq_hs_rx_wr_val(bank, AXIQ_HS_RESET_MASK); }

//! @brief      HS AXIQ RX Enable (rx_en=1, fifo_reset=0, clear_error=0)
//! @param      bank:   specifies the AXIQ bank.
//! @return     This function does not return a value.
static inline void axiq_hs_rx_enable(enum axiq_hs_bank_e bank) {
    axiq_hs_rx_wr(bank, AXIQ_HS_ENABLE_MASK, AXIQ_HS_CR_FIELD_ENABLE);
}

//! @brief      HS AXIQ RX IQ Swap Enable (rx_swap=1)
//! @param      bank:   specifies the AXIQ bank.
//! @return     This function does not return a value.
static inline void axiq_hs_rx_iqswap_on(enum axiq_hs_bank_e bank) {
    axiq_hs_rx_wr(bank, AXIQ_HS_CR_FIELD_IQSWAP, AXIQ_HS_CR_FIELD_IQSWAP);
}

//! @brief      HS AXIQ RX IQ Swap Disable (rx_swap=0)
//! @param      bank:   specifies the AXIQ bank.
//! @return     This function does not return a value.
static inline void axiq_hs_rx_iqswap_off(enum axiq_hs_bank_e bank) {
    axiq_hs_rx_wr(bank, AXIQ_HS_CR_FIELD_IQSWAP, AXIQ_HS_CR_FIELD_IQSWAP_OFF);
}

//! @brief      HS AXIQ RX IQ Swap Configure
//! @param      bank:   specifies the AXIQ bank.
//! @param      flag:   value to write to RX IQSWAP bit: =0x0 to disable, =0x1 to enable
//! @return     This function does not return a value.
static inline void axiq_hs_rx_iqswap_config(enum axiq_hs_bank_e bank, uint32_t flag) {
    axiq_hs_rx_wr(bank, AXIQ_HS_CR_FIELD_IQSWAP, (flag) << (AXIQ_HS_CR_SHIFT_IQSWAP));
}

//! @brief      Write the TX FIFO control bits in the AXIQ control register.
//! @param      bank    specifies the AXIQ bank.
//! @param      mask    right-justified bit mask specifying the control bits to write.
//! @param      ctrl    the right-justified control value to write.
//! @return     This function does not return a value.
static inline void axiq_hs_tx_wr(enum axiq_hs_bank_e bank, uint32_t mask, uint32_t ctrl) {
    iowr(AXIQ_HS_TX_CR(bank), (mask) << AXIQ_HS_CR_TX_SHIFT, (ctrl) << AXIQ_HS_CR_TX_SHIFT);
}

//! @brief      HS AXIQ TX Enable (rx_en=0).
//! @param      bank:   specifies the AXIQ bank.
//! @return     This function does not return a value.
static inline void axiq_hs_tx_disable(enum axiq_hs_bank_e bank) {
    axiq_hs_tx_wr(bank, AXIQ_HS_CR_FIELD_ENABLE, AXIQ_HS_CR_FIELD_DISABLE);
}

//! @brief      HS AXIQ TX Reset (fifo_reset=1, clear_error=1)
//! @param      bank:   specifies the AXIQ bank.
//! @return     This function does not return a value.
static inline void axiq_hs_tx_reset(enum axiq_hs_bank_e bank) { axiq_hs_tx_wr(bank, AXIQ_HS_RESET_MASK, AXIQ_HS_RESET_MASK); }

//! @brief      HS AXIQ TX Enable (rx_en=1, fifo_reset=0, clear_error=0)
//! @param      bank:   specifies the AXIQ bank.
//! @return     This function does not return a value.
static inline void axiq_hs_tx_enable(enum axiq_hs_bank_e bank) {
    axiq_hs_tx_wr(bank, AXIQ_HS_ENABLE_MASK, AXIQ_HS_CR_FIELD_ENABLE);
}

//! @brief      HS AXIQ TX IQ Swap Enable (tx_swap=1)
//! @param      bank:   specifies the AXIQ bank.
//! @return     This function does not return a value.
static inline void axiq_hs_tx_iqswap_on(enum axiq_hs_bank_e bank) {
    axiq_hs_tx_wr(bank, AXIQ_HS_CR_FIELD_IQSWAP, AXIQ_HS_CR_FIELD_IQSWAP);
}

//! @brief      HS AXIQ TX IQ Swap Disable (tx_swap=0)
//! @param      bank:   specifies the AXIQ bank.
//! @return     This function does not return a value.
static inline void axiq_hs_tx_iqswap_off(enum axiq_hs_bank_e bank) {
    axiq_hs_tx_wr(bank, AXIQ_HS_CR_FIELD_IQSWAP, AXIQ_HS_CR_FIELD_IQSWAP_OFF);
}

//! @brief      HS AXIQ TX IQ Swap Configure
//! @param      bank:   specifies the AXIQ bank.
//! @param      flag:   value to write to TX IQSWAP bit: =0x0 to disable, =0x1 to enable
//! @return     This function does not return a value.
static inline void axiq_hs_tx_iqswap_config(enum axiq_hs_bank_e bank, uint32_t flag) {
    axiq_hs_tx_wr(bank, AXIQ_HS_CR_FIELD_IQSWAP, (flag) << (AXIQ_HS_CR_SHIFT_IQSWAP));
}

//! @brief      Read HS AXIQ RX Max/Sum/Num GPIN.
//! @param      bank    specifies the AXIQ bank.
//! @return     The value of the AXIQ bank status register bit field.
static inline uint32_t axiq_hs_rx_read_max(enum axiq_hs_bank_e bank) { return iord(AXIQ_HS_RX_MAX_GPIN(bank)); }

#pragma cplusplus on
static inline void axiq_hs_fifo_read(enum axiq_hs_bank_e bank, enum axiq_hs_fifo_e fifo, uint32_t ctrl, void *data, size_t size) {
    dmac_enable(ctrl | AXIQ_FIFO_HS_RD | AXIQ_HS_CHAN_RX(bank, fifo), size, AXIQ_HS_ADDR_RX(bank, fifo), data);
}

static inline void axiq_hs_fifo_read(enum axiq_hs_bank_e bank, enum axiq_hs_fifo_e fifo, void *data, size_t size) {
    dmac_enable(AXIQ_FIFO_HS_RD | AXIQ_HS_CHAN_RX(bank, fifo), size, AXIQ_HS_ADDR_RX(bank, fifo), data);
}

static inline void axiq_hs_fifo_read(struct axiq_hs_fifo_t *axiq, uint32_t ctrl, void *data, size_t size) {
    dmac_enable(ctrl | AXIQ_FIFO_HS_RD | axiq->chan, size, axiq->addr, data);
}

#pragma cplusplus reset

#pragma cplusplus on

static inline void axiq_hs_fifo_write(enum axiq_hs_bank_e bank, enum axiq_hs_fifo_e fifo, uint32_t ctrl, void const *data,
                                      size_t size) {
    dmac_enable(ctrl | AXIQ_FIFO_HS_WR | AXIQ_HS_CHAN_TX(bank, fifo), size, AXIQ_HS_ADDR_TX(bank, fifo), (void *)data);
}

static inline void axiq_hs_fifo_write(enum axiq_hs_bank_e bank, enum axiq_hs_fifo_e fifo, void const *data, size_t size) {
    dmac_enable(AXIQ_FIFO_HS_WR | AXIQ_HS_CHAN_TX(bank, fifo), size, AXIQ_HS_ADDR_TX(bank, fifo), (void *)data);
}

static inline void axiq_hs_fifo_write(struct axiq_hs_fifo_t *axiq, uint32_t ctrl, void const *data, size_t size) {
    dmac_enable(ctrl | AXIQ_FIFO_HS_WR | axiq->chan, size, axiq->addr, (void *)data);
}
#pragma cplusplus reset

// =============================================================================
// Logical driver interface:
// =============================================================================

// -----------------------------------------------------------------------------
// Bank-level interface:
// -----------------------------------------------------------------------------

#endif // __AXIQ_HS_H__
