// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#ifndef TRANSMITTER_H
#define TRANSMITTER_H

#include "iqplayer_commands.h"
#include "vcpu.h"
#include "dma_tcd_fifo.h"

#include <stdint.h>

typedef struct TxDDR_lane {
    vspa_dma_hif_t dma_hif;
    cfixed16_t *base_buffer;
    cfixed16_t *enque_head;
    cfixed16_t *ready_buffer;
    dma_tcd_fifo_t tcd_fifo;
    uint32_t buffer_flags[2];
    uint32_t count_enque;
    uint32_t count_consumed;
    uint16_t dma_channel;
    uint16_t ready_buffer_count;
    uint16_t ready_buffer_offset;
} tx_ddr_pipeline_t;

typedef struct DAC_lane {
    cfixed16_t *base_buffer;
    cfixed16_t *next_buffer;
    uint32_t count_enque;

    uint32_t axi_fifo_addr;
    uint16_t axi_fifo_index;
    uint16_t dma_channel;
} dac_pipeline_t;

#define TX_MAX_LANE_COUNT 1

struct DebugStats2 {
    uint32_t afe_enq;
    uint32_t afe_compl;
    uint32_t afe_err;
    uint32_t afe_udr;
    uint32_t dfe_enq;
    uint32_t dfe_compl;
    uint32_t dfe_udr;
    uint32_t dfe_err;
};

extern tx_ddr_pipeline_t txddr[TX_MAX_LANE_COUNT];

void transmitter_init(void);
void tx_lane_setup(uint16_t lane, uint16_t channel, uint16_t oversamplePow2);

void tx_lane_prime(uint16_t lane);
void tx_lane_abort(uint16_t lane);

bool tx_insert_tcd(uint16_t lane, const vspa_dma_tcd_t *tcd);
void dac_dma_complete(uint16_t lane);
void tx_ddr_complete(uint16_t lane);

#endif /* IQMOS_RX_H_ */
