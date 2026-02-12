// SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
// Copyright 2019 - 2025   NXP Semiconductors

// =============================================================================
//! @file       axiq-hs.c
//! @brief      AXI I/Q FIFO High Speed driver interface.
//! @author     NXP Semiconductors.
// =============================================================================

#if defined(__LA1224__)

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <vspa/intrinsics.h>

#include "vspa.h"
#include "chip.h"
#include "iohw.h"
#include "vcpu.h"
#include "gpio.h"
#include "dmac.h"
#include "axiq.h"

//
// Bank-level interface:
//
void axiq_hs_bank_reset(enum axiq_hs_bank_e bank) {
    //@todo: add bank level reset
}

// Call this guy to properly end an RX window
void axiq_hs_rx_end(enum axiq_hs_bank_e bank) {
    // Abort any active RX DMAs and clear associated GO/ERR events
    dmac_reset(AXIQ_HS_MASK_RX(bank, AXIQ_HS_FIFO_RX));

    // AXIQ FIFO Pointer Reset and RX_EN=0 (GPOUT12/13 = 0x30)
    axiq_hs_rx_reset(bank); // clr_err=1, rst=1, en=0
}

// Call this guy to properly reset/enable RX before new RX window
void axiq_hs_fifo_rx_reset(enum axiq_hs_bank_e bank) {
    // Abort any active RX DMAs and clear associated GO/ERR events
    dmac_reset(AXIQ_HS_MASK_RX(bank, AXIQ_HS_FIFO_RX));

    // not 100% sure of this FIFO reset works correctly yet ...
    axiq_hs_rx_enable(bank); // clr_err=0, rst=0, en=1
    delay_cycle(16);
    axiq_hs_rx_reset(bank); // clr_err=1, rst=1, en=0
    delay_cycle(16);
    axiq_hs_rx_enable(bank); // clr_err=0, rst=0, en=1
}

void axiq_hs_fifo_tx_reset(enum axiq_hs_bank_e bank) {
    // TX HS AXIQ reset
    axiq_hs_tx_disable(bank); // en=0
    delay_cycle(16);
    axiq_hs_tx_reset(bank); // clr_err=1, rst=1
    delay_cycle(16);
    axiq_hs_tx_enable(bank); // clr_err=0, rst=0, en=1

    // TX HS DMA reset
    dmac_clear(AXIQ_HS_MASK_TX(bank, AXIQ_HS_FIFO_TX));
}

void axiq_hs_rx_max_reset(enum axiq_hs_bank_e bank) {
    // restart_rx_max_bit = 0
    axiq_hs_rx_wr(bank, AXIQ_HS_CR_RX_MAX_RESTART_MASK, 0);
    delay_cycle(24);
    // restart_rx_max_bit = 1, ch_out_select = 0b11
    axiq_hs_rx_wr(bank, AXIQ_HS_CR_RX_MAX_MASK, AXIQ_HS_CR_RX_MAX_MASK);
    delay_cycle(24);
}

void axiq_hs_rx_max_out_sel_sum(enum axiq_hs_bank_e bank) {
    delay_cycle(24);
    // ch_out_select = 0b10 = sum_rx_data
    axiq_hs_rx_wr(bank, AXIQ_HS_CR_RX_MAX_OUT_SEL_MASK, AXIQ_HS_CR_RX_MAX_OUT_SEL_SUM_MASK);
    delay_cycle(24);
}

void axiq_hs_rx_max_out_sel_num(enum axiq_hs_bank_e bank) {
    // ch_out_select = 0b10 = num_rx_data
    axiq_hs_rx_wr(bank, AXIQ_HS_CR_RX_MAX_OUT_SEL_MASK, AXIQ_HS_CR_RX_MAX_OUT_SEL_NUM_MASK);
    delay_cycle(16);
}

#endif
