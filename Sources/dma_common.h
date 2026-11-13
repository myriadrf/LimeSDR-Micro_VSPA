// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#ifndef DMA_COMMON_H
#define DMA_COMMON_H

#include "dmac.h"
#include "chip.h"

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

#define DDR_WR_DMA_CHANNEL_5 DMAC_CHAN_0

#define RO0_ADC_RD_DMA_CHANNEL DMAC_CHAN_1
#define RO1_ADC_RD_DMA_CHANNEL DMAC_CHAN_2
#define RX0_ADC_RD_DMA_CHANNEL DMAC_CHAN_3
#define RX1_ADC_RD_DMA_CHANNEL DMAC_CHAN_4

#define AUX_ADC_RD_DMA_CHANNEL DMAC_CHAN_5
#define RSSI_RD_DMA_CHANNEL DMAC_CHAN_6

#define DDR_RD_DMA_CHANNEL_1 DMAC_CHAN_7
#define DDR_RD_DMA_CHANNEL_2 DMAC_CHAN_8
#define DDR_RD_DMA_CHANNEL_3 DMAC_CHAN_9
#define DDR_RD_DMA_CHANNEL_4 DMAC_CHAN_10

#define TX_DAC_WR_DMA_CHANNEL DMAC_CHAN_11

#define DDR_WR_DMA_CHANNEL_1 DMAC_CHAN_12
#define DDR_WR_DMA_CHANNEL_2 DMAC_CHAN_13
#define DDR_WR_DMA_CHANNEL_3 DMAC_CHAN_14
#define DDR_WR_DMA_CHANNEL_4 DMAC_CHAN_15

#define DMEM_LINE_SIZE_BYTES 128 // 1024 bits

#define VSPA_DEFAULT_TIMEOUT 4000

// VSPA addresses in half words (16bits), DMA uses bytes (8bits)
#define VCPU_ADDR_FOR_DMA(x) (((uint32_t)x) << 1)

void dma_clear_all_bits(uint32_t mask);

#endif // DMA_COMMON_H
