// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#ifndef RECEIVER_H
#define RECEIVER_H

#include "iqplayer_commands.h"
#include "vcpu.h"
#include "vspa_dma_hif.h"
#include "afe_pipeline.h"
#include "ddr_pipeline.h"

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    VSPA_RO0,
    VSPA_RO1,
    VSPA_RX0,
    VSPA_RX1,
} e_rx_channel;

#define RX_CHANNEL_COUNT 4

// Persistent channel parameters, applied to the lane which streams the channel
typedef struct RxChannelConfig {
    uint16_t decimate_pow2;
} rx_channel_config_t;

typedef struct RxDDR_lane {
    dma_tcd_fifo_t output_blocks;
    dma_tcd_t *wip_tcd_output;
    uint64_t buf_timestamp; // timestamp of the first sample in the DDR transfer buffer being filled
    ddr_pipeline_t ddr;
    uint16_t buf_filled;
    uint16_t decimate_pow2; // copy of the selected channel's config, latched on channel select and lane prime
} rx_ddr_pipeline_t;

typedef struct ADC_lane {
    afe_pipeline_t afe;
    uint64_t phyticks; // ADC sample time (phytime ticks since stream origin) of the next buffer to complete
    uint16_t channel; // e_rx_channel
    bool sampling; // rx_allowed is raised, or scheduled to be raised
} adc_pipeline_t;

#define RX_MAX_LANE_COUNT 1

extern rx_ddr_pipeline_t rx_ddr[RX_MAX_LANE_COUNT];
extern adc_pipeline_t rx_adc[RX_MAX_LANE_COUNT];

void receiver_init(void);
int rx_select_channel(uint16_t lane, e_rx_channel channel);
int rx_set_sample_format(uint16_t lane, iq_sample_format_e format);
int rx_set_oversampling(e_rx_channel channel, uint16_t decimate_pow2);
// @return channel's decimation as power of 2, or negative lime_Result on error
int rx_get_oversampling(e_rx_channel channel);

void rx_lane_prime(uint16_t lane);
void rx_lane_stop(uint16_t lane);
void rx_stream_started(void);

void adc_dma_complete(uint16_t lane);
void rx_ddr_dma_complete(uint16_t lane);

cfloat32_t *rx_dft_measure(e_rx_channel channel, int32_t fcw);
void host_submitted_rx_tcd(uint16_t lane);

#endif /* IQMOS_RX_H_ */
