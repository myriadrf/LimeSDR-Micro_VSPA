// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#ifndef RECEIVER_H
#define RECEIVER_H

#include "iqplayer_commands.h"
#include "vcpu.h"
#include "vspa_dma_hif.h"

#include <stdint.h>

typedef enum {
    VSPA_RO0,
    VSPA_RO1,
    VSPA_RX0,
    VSPA_RX1,
} e_rx_channel;

typedef struct RxBufferMetaData {
    uint32_t flags;
} rx_meta_t;

typedef struct RxDDR_lane {
    vspa_dma_hif_t dma;
    cfixed16_t *base_buffer;
    cfixed16_t *write_head;
    rx_meta_t meta[2];
    uint32_t count_dmac_enque;
    uint32_t count_dmac_complete;
    uint16_t dma_channel;
    uint16_t buf_filled;
    uint16_t decimate_pow2;
} rx_ddr_pipeline_t;

typedef struct ADC_lane {
    cfixed16_t *base_buffer;
    cfixed16_t *next_completion_buffer;
    uint32_t axi_fifo_addr;
    uint32_t count_dmac_complete;
    uint16_t axi_fifo_index;
    uint16_t dma_channel;
} adc_pipeline_t;

#define RX_MAX_LANE_COUNT 2

extern rx_ddr_pipeline_t rxddr[RX_MAX_LANE_COUNT];
extern adc_pipeline_t adc[RX_MAX_LANE_COUNT];

void receiver_init(void);
int rx_select_channel(uint16_t lane, e_rx_channel channel);
int rx_set_oversampling(uint16_t lane, uint16_t decimate_pow2);

void rx_lane_prime(uint16_t lane);
void rx_lane_stop(uint16_t lane);

void adc_dma_complete(uint16_t lane);
void ddr_dma_complete(uint16_t lane);

#endif /* IQMOS_RX_H_ */
