// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#ifndef TRANSMITTER_H
#define TRANSMITTER_H

#include <stdint.h>

#include "opstatus.h"

typedef struct TxControl {
    uint32_t ddr_enabled;
    uint32_t generate_tone;
    uint32_t host_flow_control_disable;
    uint32_t burst_start_bytes;
    uint32_t burst_end_bytes;
    uint32_t ddr_rd_dma_ch_nb;
    uint32_t ddr_rd_dma_ch_mask;
    uint32_t ddr_rd_dma_mBurst;
    uint32_t burst_active;
} tx_control_t;

typedef struct TxConfig {
    uint32_t ddr_base_address;
    uint32_t ddr_size;
    uint32_t ddr_step;
    uint32_t oversample;
} tx_config_t;

void InitializeTx(void);

void ProcessTx(void);

void TxHostFIFO(uint32_t addr, uint32_t size);
lime_Result TxConfigure(uint32_t oversample);

lime_Result TxControl(const tx_config_t *ctrl);

void TxSetBurstSize(uint32_t bytes);

#endif // TRANSMITTER_H
