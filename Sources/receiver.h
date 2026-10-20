// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#ifndef RECEIVER_H
#define RECEIVER_H

#include "iqplayer_commands.h"
#include "vcpu.h"
#include "vspa_dma_hif.h"

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    VSPA_RO0,
    VSPA_RO1,
    VSPA_RX0,
    VSPA_RX1,
} e_rx_channel;

typedef struct RxBufferMetaData {
    uint64_t timestamp; // first sample's timestamp, samples count since stream start (after decimation)
    uint32_t size; // bytes of contiguous samples written into the TCD buffer
    uint32_t flags;
} rx_meta_t;

typedef struct RxDDR_lane {
    vspa_dma_hif_t dma;
    dma_tcd_fifo_t output_blocks;
    rx_meta_t meta[2];
    dma_tcd_t wip_tcd; // working copy of input tcd, so the original could be recirculated.
    dma_tcd_t *wip_tcd_output;
    cfixed16_t *base_buffer;
    uint64_t buf_timestamp; // timestamp of the first sample in the DDR transfer buffer being filled
    uint32_t count_dmac_enque;
    uint32_t count_dmac_complete;
    uint16_t dma_channel;
    uint16_t buf_filled;
    uint16_t decimate_pow2;
} rx_ddr_pipeline_t;

typedef struct ADC_lane {
    cfixed16_t *base_buffer;
    uint32_t axi_fifo_addr;
    uint32_t count_dmac_enque;
    uint32_t count_dmac_complete;
    uint64_t phyticks; // ADC sample time (phytime ticks since stream origin) of the next buffer to complete
    uint16_t axi_fifo_index;
    uint16_t dma_channel;
    uint16_t phytimer_id;
    bool sampling; // rx_allowed is raised, or scheduled to be raised
} adc_pipeline_t;

#define RX_MAX_LANE_COUNT 1

extern rx_ddr_pipeline_t rx_ddr[RX_MAX_LANE_COUNT];
extern adc_pipeline_t rx_adc[RX_MAX_LANE_COUNT];

void receiver_init(void);
int rx_select_channel(uint16_t lane, e_rx_channel channel);
int rx_set_oversampling(uint16_t lane, uint16_t decimate_pow2);
int rx_set_sample_format(uint16_t lane, iq_sample_format_e format);

void rx_lane_prime(uint16_t lane);
void rx_lane_stop(uint16_t lane);
void rx_stream_started(void);

void adc_dma_complete(uint16_t lane);
void rx_ddr_dma_complete(uint16_t lane);

vspa_complex_float32 *rx_fft(uint16_t channel);
cfixed16_t *capture_adc(uint16_t channel);
void host_submitted_rx_tcd(uint16_t lane);

#endif /* IQMOS_RX_H_ */
