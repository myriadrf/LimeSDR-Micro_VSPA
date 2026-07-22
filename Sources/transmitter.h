// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#ifndef TRANSMITTER_H
#define TRANSMITTER_H

#include <stdint.h>

#include "opstatus.h"
#include "dma_table.h"

typedef struct TxControl {
    uint32_t ddr_enabled;
    uint32_t generate_tone;
    uint32_t ddr_rd_dma_mBurst;
    uint32_t dma_table_loop;
} tx_control_t;

typedef struct TxConfig {
    uint32_t oversample;
} tx_config_t;

typedef struct Tx_DMA_hif {
    uint64_t timestamp;
    uint32_t la9310_mem_address;
    uint32_t size;
    uint32_t flags;
    uint32_t loop_counter;
} tx_dma_hif_t;

void InitializeTx(void);

lime_Result TxConfigure(uint32_t oversample);
lime_Result TxControl(const tx_config_t *ctrl);
lime_Result TxDMASubmit(void);

lime_Result TxDDR_control(uint64_t msg64);
void TxTone_control(uint64_t msg64);
void HostProducedEvent(void);

void OnDACWrite_Completed(void);
void OnDDRRD_Completed(void);

bool TxBurstCompleteAndReady(void);

#endif // TRANSMITTER_H
