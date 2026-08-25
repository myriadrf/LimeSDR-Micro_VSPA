// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#ifndef RECEIVER_H
#define RECEIVER_H

#include "iqplayer_commands.h"
#include "vcpu.h"
#include "dma_tcd_fifo.h"

#include <stdint.h>

typedef enum {
    VSPA_RO0,
    VSPA_RO1,
    VSPA_RX0,
    VSPA_RX1,
} e_rx_channel;

typedef struct RxDDR_lane {
    vspa_dma_hif_t rx_host_if;
    cfixed16_t *base_buffer;
    cfixed16_t *write_head;
    dma_tcd_fifo_t tcd_fifo;
    uint32_t count_enque;
    uint16_t dma_channel;
    uint16_t buf_filled;
    uint16_t decimate_pow2;
} rx_ddr_pipeline_t;

typedef struct ADC_lane {
    cfixed16_t *base_buffer;
    cfixed16_t *next_completion_buffer;
    uint32_t axi_fifo_addr;
    uint32_t completion_count;
    uint16_t axi_fifo_index;
    uint16_t dma_channel;
} adc_pipeline_t;

#define RX_MAX_LANE_COUNT 1

extern rx_ddr_pipeline_t rxddr[RX_MAX_LANE_COUNT];

struct DebugStats {
    uint32_t adc_enq;
    uint32_t adc_compl;
    uint32_t ddr_enq;
    uint32_t ddr_compl;
    uint32_t ddr_ovr;
    uint32_t adc_err;
    uint32_t ddr_err;
};

void receiver_init(void);
void rx_setup_channel(uint16_t lane, e_rx_channel channel, uint16_t oversample_pow2);

void rx_lane_prime(uint16_t lane);
void rx_lane_stop(uint16_t lane);

bool rx_insert_tcd(uint16_t lane, const vspa_dma_tcd_t *tcd);
void adc_dma_complete(uint16_t lane);
void ddr_dma_complete(uint16_t lane);

#endif /* IQMOS_RX_H_ */
