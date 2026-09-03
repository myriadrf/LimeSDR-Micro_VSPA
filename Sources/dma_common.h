// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#ifndef DMA_COMMON_H
#define DMA_COMMON_H

#include "dmac.h"

#include <stdint.h>
#include <stdbool.h>

// The ADC/DAC AXI FIFO is currently sized at 32 entries x 128-bit/entry:
// enough space to buffer up to two 16-beat bursts from AXI.
// The DMA interfaces primarily monitor the FIFO level and triggers the DMA
// whenever the FIFO level has gone beyond a programmed threshold.

// The TX completes it’s transmission when two events occur:
// 1. A falling edge is detected on the 'tx_dma_allowed' signal (PHYTimer T11 trigger).
// 2. TX FIFO receives a ptr_rst_req from the DMA (DMAC_PRST_REQ).
// Both events must occur (in any order).
// Note: 'ptr_rst_req' should be done only once per 'tx_dma_allowed' session,
// multiple subsequent 'ptr_rst_req' calls will cause DMA to get stuck.
// To unstuck, 'tx_dma_allowed' has to be triggered twice.

#define MAX_DMA_ENQ 2

#define DDR_WR_DMA_CHANNEL_5 0x0

#define RO0_ADC_RD_DMA_CHANNEL 0x1
#define R01_ADC_RD_DMA_CHANNEL 0x2
#define RX0_ADC_RD_DMA_CHANNEL 0x3
#define RX1_ADC_RD_DMA_CHANNEL 0x4

#define AUX_ADC_RD_DMA_CHANNEL 0x5
#define RSSI_RD_DMA_CHANNEL 0x6

#define DDR_RD_DMA_CHANNEL_1 0x7
#define DDR_RD_DMA_CHANNEL_2 0x8
#define DDR_RD_DMA_CHANNEL_3 0x9
#define DDR_RD_DMA_CHANNEL_4 0xa

#define TX_DAC_WR_DMA_CHANNEL 0xb

#define DDR_WR_DMA_CHANNEL_1 0xc
#define DDR_WR_DMA_CHANNEL_2 0xd
#define DDR_WR_DMA_CHANNEL_3 0xe
#define DDR_WR_DMA_CHANNEL_4 0xf

#define DMEM_LINE_SIZE_BYTES 128 // 1024 bits

// VSPA addresing is in 16bit granularity
#define VSPA_HALF_WORDS(x) (2 * (uint32_t)x)

// VSPA addresses in half words (16bits), DMA uses bytes (8bits)
#define VCPU_ADDR_FOR_DMA(x) (((uint32_t)x) << 1)

uint32_t dma_chan_mask(uint32_t dma_channel, uint8_t nb_dma);

static bool did_timeout = false;

#define WAIT_TIMEOUT_R(cond, timeout_cycles) \
    do {                                     \
        did_timeout = false;                 \
        uint32_t timeout = timeout_cycles;   \
        do {                                 \
        } while (!(cond) && --timeout);      \
        did_timeout = timeout == 0;          \
    } while (0)

#endif // DMA_COMMON_H
