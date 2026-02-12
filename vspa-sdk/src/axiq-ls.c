// SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
// Copyright 2019 - 2025   NXP Semiconductors

// =============================================================================
//! @file       axiq-ls.c
//! @brief      AXI I/Q FIFO Low Speed driver interface.
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

// Call this guy to properly end an certain RX window for one path
void axiq_ls_rx_onepath_end(enum axiq_ls_bank_e bank, enum axiq_ls_fifo_rx_e fifo) {
    // Abort any active RX DMAs and clear associated GO/ERR events
    dmac_reset(AXIQ_LS_MASK_RX(bank, fifo));

    // AXIQ FIFO Pointer Reset and RX_EN=0 (GPOUT12/13 = 0x30)
    axiq_ls_rx_reset(bank, fifo); // clr_err=1, rst=1, en=0
}

// Call this guy to properly end an RX window
void axiq_ls_rx_end(enum axiq_ls_bank_e bank) {
    // Abort any active RX DMAs and clear associated GO/ERR events
    dmac_reset(AXIQ_LS_MASK_BOTH_RX(bank));

    // AXIQ FIFO Pointer Reset and RX_EN=0 (GPOUT12/13 = 0x30)
    axiq_ls_rx_reset(bank); // clr_err=1, rst=1, en=0
}

// Call this guy to properly reset/enable both RX paths before new RX window
void axiq_ls_fifo_rx_reset(enum axiq_ls_bank_e bank) {
    // Abort any active RX DMAs and clear associated GO/ERR events
    dmac_reset(AXIQ_LS_MASK_BOTH_RX(bank));

    // not 100% sure of this FIFO reset works correctly yet ...
    axiq_ls_rx_enable(bank); // clr_err=0, rst=0, en=1
    delay_cycle(16);
    axiq_ls_rx_reset(bank); // clr_err=1, rst=1, en=0
    delay_cycle(16);
    axiq_ls_rx_enable(bank); // clr_err=0, rst=0, en=1
}

void axiq_ls_fifo_tx_reset(enum axiq_ls_bank_e bank) {
    // TX LS AXIQ reset
    axiq_ls_tx_disable(bank); // en=0
    delay_cycle(16);
    axiq_ls_tx_reset(bank); // clr_err=1, rst=1
    delay_cycle(16);
    axiq_ls_tx_enable(bank); // clr_err=0, rst=0, en=1

    // TX LS DMA reset
    dmac_clear(AXIQ_LS_MASK_BOTH_TX(bank));
}

void axiq_ls_rx_max_reset(enum axiq_ls_bank_e bank) {
    // restart_rx_max_bit = 0
    axiq_ls_rx_wr(bank, AXIQ_LS_CR_RX_MAX_RESTART_MASK, 0);
    delay_cycle(24);
    // restart_rx_max_bit = 1, ch_out_select = 0b11
    axiq_ls_rx_wr(bank, AXIQ_LS_CR_RX_MAX_MASK, AXIQ_LS_CR_RX_MAX_MASK);
    delay_cycle(24);
}

void axiq_ls_rx0_max_out_sel_sum(enum axiq_ls_bank_e bank) {
    delay_cycle(24);
    // ch_out_select = 0b10 = sum_rx_data
    axiq_ls_rx_wr(bank, AXIQ_LS_CR_RX_MAX_OUT_SEL_MASK, AXIQ_LS_CR_RX_MAX_OUT_SEL_SUM0_MASK);
    delay_cycle(24);
}

void axiq_ls_rx1_max_out_sel_sum(enum axiq_ls_bank_e bank) {
    delay_cycle(24);
    // ch_out_select = 0b10 = sum_rx_data
    axiq_ls_rx_wr(bank, AXIQ_LS_CR_RX_MAX_OUT_SEL_MASK, AXIQ_LS_CR_RX_MAX_OUT_SEL_SUM1_MASK);
    delay_cycle(24);
}

void axiq_ls_rx0_max_out_sel_num(enum axiq_ls_bank_e bank) {
    // ch_out_select = 0b10 = num_rx_data
    axiq_ls_rx_wr(bank, AXIQ_LS_CR_RX_MAX_OUT_SEL_MASK, AXIQ_LS_CR_RX_MAX_OUT_SEL_NUM0_MASK);
    delay_cycle(16);
}

void axiq_ls_rx1_max_out_sel_num(enum axiq_ls_bank_e bank) {
    // ch_out_select = 0b10 = num_rx_data
    axiq_ls_rx_wr(bank, AXIQ_LS_CR_RX_MAX_OUT_SEL_MASK, AXIQ_LS_CR_RX_MAX_OUT_SEL_NUM1_MASK);
    delay_cycle(16);
}

#endif
