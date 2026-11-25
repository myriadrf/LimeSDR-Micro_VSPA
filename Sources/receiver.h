// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#ifndef RECEIVER_H
#define RECEIVER_H

#include "fw_config.h"

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

typedef enum { RX_WORK_DDR, RX_WORK_DFT } rx_work_path_t;

typedef struct RxDDR_lane {
    dma_tcd_t *wip_tcd_output; // back of rx_output_blocks[pipe], being filled
    uint64_t buf_timestamp; // timestamp of the first sample in the DDR transfer buffer being filled
    rx_work_path_t work_path;
} rx_pipeline_t;

typedef struct ADC_lane {
    afe_pipeline_t afe;
    uint64_t phyticks; // ADC sample time (phytime ticks since stream origin) of the next buffer to complete
    uint16_t channel; // e_rx_channel
    bool sampling; // rx_allowed is raised, or scheduled to be raised
} adc_pipeline_t;

extern axiq_config_t rx_axiq_configs[RX_PIPE_COUNT];
extern vspa_channel_config_t rx_channel_cfg[RX_CHANNEL_COUNT];
extern vspa_dma_hif_t rx_hdma[RX_PIPE_COUNT];
extern dma_tcd_fifo_t rx_output_blocks[RX_PIPE_COUNT];

extern ddr_pipeline_t rx_ddr[RX_PIPE_COUNT];
extern adc_pipeline_t rx_adc[RX_PIPE_COUNT];

void receiver_init(void);
int rx_select_channel(uint16_t lane, e_rx_channel channel);
int rx_set_sample_format(uint16_t lane, iq_sample_format_e format);

void rx_lane_prime(uint16_t lane);
void rx_lane_stop(uint16_t lane);
void rx_stream_started(void);

void rx_ddr_dma_complete(uint16_t lane);

// DFT measurement of a single frequency on an Rx channel, done on pipe 0 over several GO events.
// @param fcw frequency control word, frequency / 2^32 as a fraction of the host side sample rate
int rx_dft_start(e_rx_channel channel, int32_t fcw);

void host_submitted_rx_tcd(uint16_t lane);

void rx_pipe_process(uint16_t pipe);

#endif /* IQMOS_RX_H_ */
