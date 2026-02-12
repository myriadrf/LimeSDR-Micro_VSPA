// SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
// Copyright 2019 - 2025   NXP Semiconductors

// =============================================================================
//! @file       axiq-la1224.h
//! @brief      LA1224 AXI I/Q FIFO driver interface.
//! @author     NXP Semiconductors.
//!
//! @details    This file provides a software interface for the 3 AXI I/Q
//!             hardware interfaces in LA1224.
//!             The 2 low-speed AXIQ interfaces include 2 receive paths
//!             and 2 transmit paths.
//!             The high-speed AXIQ interface is perceived as a 2-bank module
//!             with 1 receive path and 1 transmit path per bank.
//!
// =============================================================================

#ifndef __AXIQ_LA1224_H__
#define __AXIQ_LA1224_H__

// =============================================================================
// Geul LS-DCS <-> AXIQ connections
// =============================================================================
#define GEUL_ACTIVE_RX_PER_AXIQ 2
#define GEUL_ACTIVE_TX_PER_AXIQ 2
#define GEUL_ACTIVE_RX_PER_AXIQ_MASK ((0x1 << (GEUL_ACTIVE_RX_PER_AXIQ)) - 1)
#define GEUL_ACTIVE_TX_PER_AXIQ_MASK ((0x1 << (GEUL_ACTIVE_TX_PER_AXIQ)) - 1)

// =============================================================================
// AXIQ interface type:
// =============================================================================

//! @brief      AXIQ interface type enumeration.
enum axiq_type_e {
    AXIQ_TYPE_LS, //!< Low-speed AXIQ interface.
    AXIQ_TYPE_HS, //!< High-speed AXIQ interface.
    AXIQ_TYPE_CNT //!< Total number of AXIQ interface types.
};

// =============================================================================
// Low-speed AXIQ interface:
// =============================================================================

// -----------------------------------------------------------------------------
// AXIQ banks:
// -----------------------------------------------------------------------------

//! @brief      Low-speed AXIQ interface bank enumeration.
enum axiq_ls_bank_e {
    AXIQ_LS_BANK_0,  //!< Low-speed AXIQ interface #0 (2x2).
    AXIQ_LS_BANK_1,  //!< Low-speed AXIQ interface #1 (2x2).
    AXIQ_LS_BANK_CNT //!< Total number of low-speed AXIQ banks.
};

// -----------------------------------------------------------------------------
// Control register:
// -----------------------------------------------------------------------------

//! @brief      AXIQ RX control register address.
#define AXIQ_LS_RX_CR(bank) GPO(((bank)*2) + GPIO_AXIQ_L0_CR0)

//! @brief      AXIQ TX control register address.
#define AXIQ_LS_TX_CR(bank) GPO(((bank)*2) + GPIO_AXIQ_L0_CR1)

// -----------------------------------------------------------------------------
// Status register:
// -----------------------------------------------------------------------------

//! @brief      AXIQ RX status register address.
#define AXIQ_LS_RX_SR(bank) GPI(((bank)*3) + GPIO_AXIQ_L0_SR0)

//! @brief      AXIQ TX status register address.
#define AXIQ_LS_TX_SR(bank) GPI(((bank)*3) + GPIO_AXIQ_L0_SR1)

//! @brief      DMA channel number for a given RX FIFO.
//! @param      bank    specifies the AXIQ bank.
//! @param      fifo    specifies the AXIQ RX FIFO.
//! @return     The DMA channel number for the RX FIFO @a fifo.
#define AXIQ_LS_CHAN_RX(bank, fifo) (((bank)*5) + (fifo) + 1)

//! @brief      DMA channel mask for a given RX FIFO.
//! @param      bank    specifies the AXIQ bank.
//! @param      fifo    specifies the AXIQ RX FIFO.
//! @return     The DMA channel number for the RX FIFO @a fifo.
#define AXIQ_LS_MASK_RX(bank, fifo) (0b1 << AXIQ_LS_CHAN_RX(bank, fifo))

//! @brief      DMA channel mask for both RX FIFOs in a given AXIQ bank.
//! @param      bank    specifies the AXIQ bank.
//! @return     DMA channel mask for both RX FIFOs in given AXIQ bank
#define AXIQ_LS_MASK_BOTH_RX(bank) ((GEUL_ACTIVE_RX_PER_AXIQ_MASK) << AXIQ_LS_CHAN_RX(bank, 0))

//! @brief      AXI address for a given RX FIFO.
//! @param      bank    specifies the AXIQ bank.
//! @param      fifo    specifies the AXIQ RX FIFO.
//! @return     The AXI address for the RX FIFO @a fifo.
#define AXIQ_LS_ADDR_RX(bank, fifo) (BASE_AXIQ(bank) + ((fifo)*0x1000))

//! @brief      DMA channel for a given TX FIFO.
//! @param      bank    specifies the AXIQ bank.
//! @param      fifo    specifies the AXIQ TX FIFO.
//! @return     The DMA channel number for the TX FIFO @a fifo.
#define AXIQ_LS_CHAN_TX(bank, fifo) (((bank)*5) + (fifo) + 4)

//! @brief      DMA channel mask for a given TX FIFO.
//! @param      bank    specifies the AXIQ bank.
//! @param      fifo    specifies the AXIQ TX FIFO.
//! @return     The DMA channel number for the TX FIFO @a fifo.
#define AXIQ_LS_MASK_TX(bank, fifo) (0b1 << AXIQ_LS_CHAN_TX(bank, fifo))

//! @brief      DMA channel mask for both TX FIFOs in a given AXIQ bank.
//! @param      bank    specifies the AXIQ bank.
//! @return     DMA channel mask for both TX FIFOs in given AXIQ bank
#define AXIQ_LS_MASK_BOTH_TX(bank) ((GEUL_ACTIVE_TX_PER_AXIQ_MASK) << AXIQ_LS_CHAN_TX(bank, 0))

//! @brief      AXI address for a given RX FIFO.
//! @param      bank    specifies the AXIQ bank.
//! @param      fifo    specifies the AXIQ RX FIFO.
//! @return     The AXI address for the RX FIFO @a fifo.
#define AXIQ_LS_ADDR_TX(bank, fifo) (BASE_AXIQ(bank) + ((fifo)*0x1000))

#include "axiq-ls.h"

// =============================================================================
// High-speed AXIQ interface:
// =============================================================================

// -----------------------------------------------------------------------------
// AXIQ banks:
// -----------------------------------------------------------------------------

//! @brief      High-speed AXIQ interface bank enumeration.
enum axiq_hs_bank_e {
    AXIQ_HS_BANK_0,  //!< High-speed AXIQ interface #0 (1x1).
    AXIQ_HS_BANK_1,  //!< High-speed AXIQ interface #1 (1x1).
    AXIQ_HS_BANK_CNT //!< Total number of high-speed AXIQ banks.
};

// -----------------------------------------------------------------------------
// Control register:
// -----------------------------------------------------------------------------

//! @brief      AXIQ RX control register address.
#define AXIQ_HS_RX_CR(bank) GPO(GPIO_AXIQ_HS_CR0 + (bank))

//! @brief      AXIQ TX control register address.
#define AXIQ_HS_TX_CR(bank) GPO(GPIO_AXIQ_HS_CR0 + (bank))

// -----------------------------------------------------------------------------
// Status register:
// -----------------------------------------------------------------------------

//! @brief      AXIQ RX status register address.
#define AXIQ_HS_RX_SR(bank) GPI(GPIO_AXIQ_HS_SR1)

//! @brief      AXIQ TX status register address.
#define AXIQ_HS_TX_SR(bank) GPI(GPIO_AXIQ_HS_SR1)

//! @brief      HS AXIQ RX Max/Num/Sum GPIN.
#define AXIQ_HS_RX_MAX_GPIN(bank) GPI(((bank)*2) + GPIO_AXIQ_HS_SR0)

//! @brief      DMA channel number for a given RX FIFO.
//! @param      bank    specifies the AXIQ bank.
//! @param      fifo    specifies the AXIQ RX FIFO.
//! @return     The DMA channel number for the RX FIFO @a fifo.
#define AXIQ_HS_CHAN_RX(bank, fifo) ((bank) + 11)

//! @brief      DMA channel mask for a given RX FIFO.
//! @param      bank    specifies the AXIQ bank.
//! @param      fifo    specifies the AXIQ RX FIFO.
//! @return     The DMA channel number for the RX FIFO @a fifo.
#define AXIQ_HS_MASK_RX(bank, fifo) (0b1 << AXIQ_HS_CHAN_RX(bank, fifo))

//! @brief      AXI address for a given RX FIFO.
//! @param      bank    specifies the AXIQ bank.
//! @param      fifo    specifies the AXIQ RX FIFO.
//! @return     The AXI address for the RX FIFO @a fifo.
#define AXIQ_HS_ADDR_RX(bank, fifo) (BASE_AXIQ_HS + ((bank)*0x8000))

//! @brief      DMA channel for a given TX FIFO.
//! @param      bank    specifies the AXIQ bank.
//! @param      fifo    specifies the AXIQ TX FIFO.
//! @return     The DMA channel number for the TX FIFO @a fifo.
#define AXIQ_HS_CHAN_TX(bank, fifo) ((bank) + 14)

//! @brief      DMA channel mask for a given TX FIFO.
//! @param      bank    specifies the AXIQ bank.
//! @param      fifo    specifies the AXIQ TX FIFO.
//! @return     The DMA channel number for the TX FIFO @a fifo.
#define AXIQ_HS_MASK_TX(bank, fifo) (0b1 << AXIQ_HS_CHAN_TX(bank, fifo))

//! @brief      AXI address for a given TX FIFO.
//! @param      bank    specifies the AXIQ bank.
//! @param      fifo    specifies the AXIQ TX FIFO.
//! @return     The AXI address for the TX FIFO @a fifo.
#define AXIQ_HS_ADDR_TX(bank, fifo) (BASE_AXIQ_HS + ((bank)*0x8000))

#include "axiq-hs.h"
#include "axiq-ls.h"

#endif // __AXIQ_LA1224_H__
