// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#ifndef TRANSMITTER_H
#define TRANSMITTER_H

#include "fw_config.h"

#include "iqplayer_commands.h"
#include "vcpu.h"
#include "vspa_dma_hif.h"
#include "tone_generator.h"
#include "afe_pipeline.h"
#include "ddr_pipeline.h"

#include <stdint.h>

// Persistent channel parameters, applied to the lane which streams the channel
typedef struct TxChannelConfig {
    uint16_t interpolate_pow2;
} tx_channel_config_t;

typedef struct TxBufferMetaData {
    uint32_t phytime;
    uint32_t flags;
    uint16_t samples; // DDR buffer: samples the host provided, the rest of the buffer is zero padding
} tx_meta_t;

// Tx DDR (host memory) read ring. The DAC needs a new 512-sample buffer every 16.7 us at 30.72 MS/s; with 2 ring
// buffers (33 us) a PCIe read latency spike, or a read rate a little below the sample rate during a long burst, leaves
// no DDR buffer ready inside the burst and the DAC timeline slips. 6 buffers (100 us), as in Lukas Brechtel's ring on
// the oai branch: at most MAX_DMA_ENQ reads are in flight on the DMA channel, the rest of the ring is read ahead as
// reads complete and refilled in the gap before the next burst. 6 is not a power of 2 and 2^32 is not a multiple of 6,
// so the ring slots are kept separately (enq_slot/cpl_slot/consumed_slot, as in Lukas's ring); the counters only count.
#define TX_DDR_RING 6

typedef struct TxDDR_lane {
    ddr_pipeline_t ddr; // its meta[] and wip_tcd are unused, Tx keeps its own meta ring
    tx_meta_t meta[TX_DDR_RING];
    uint16_t ready_buffer_offset;
    uint8_t enq_slot; // ring slot of the next DDR read (count_dmac_enque mod ring_len)
    uint8_t cpl_slot; // ring slot of the next DDR read to complete (count_dmac_complete mod ring_len)
    uint8_t consumed_slot; // ring slot of the oldest ready buffer (buffers_consumed mod ring_len)
    uint8_t ring_len; // slots in use, TX_DDR_RING or less when the interpolator takes slots as its work memory
    // DAC buffers still to interpolate from zero input at a burst's end, to output the samples held in the interpolator's
    // filter history. While non-zero, the burst's last DDR buffer stays the current ready buffer, zeroed as their input.
    uint8_t flush_remaining;
} tx_ddr_pipeline_t;

static inline uint8_t tx_ddr_next_slot(const tx_ddr_pipeline_t *tx, uint8_t slot) {
    return slot + 1 == tx->ring_len ? 0 : slot + 1;
}

typedef struct DAC_lane {
    afe_pipeline_t afe;
    tx_meta_t meta[2];
    uint16_t channel;
    uint32_t expected_end_phytime; // needed for terminating transmissions without timestamps
    uint32_t start_phytime; // phytime of the current transmission window's START
    uint32_t end_go_phytime; // phytime of the GO_1 event that closes the current transmission window
    bool end_scheduled;
    bool end_deferred; // END of a short burst: scheduled when its DAC buffer completes, after the START fired
    bool in_progress;
    bool force_start;
} dac_pipeline_t;

typedef struct TxPipeline {
    bool generate_tone;
} tx_pipeline_t;

extern tx_ddr_pipeline_t txddr[TX_MAX_LANE_COUNT];
extern tone_state_t tx_tone_state[TX_MAX_LANE_COUNT];

void transmitter_init(void);
void tx_lane_setup(uint16_t lane, uint16_t channel);
int tx_set_sample_format(uint16_t lane, iq_sample_format_e format);
int tx_set_oversampling(uint16_t channel, uint16_t oversample_pow2);
// @return channel's interpolation as power of 2, or negative lime_Result on error
int tx_get_oversampling(uint16_t channel);

void tx_lane_ddr_enable(uint16_t lane, bool enable);

void tx_lane_prime(uint16_t lane);
void tx_lane_abort(uint16_t lane);

void dac_dma_complete(uint16_t lane);
void tx_ddr_complete(uint16_t lane);
void tx_lane_try_ddr_enqueue(tx_ddr_pipeline_t *tx, bool vcpu_trig);

int tx_tone_enable(uint16_t lane, bool enable);
void host_submitted_tx_tcd(uint16_t lane);
void tx_check_axiq_status(void);

void tx_deffered_start(void);
void tx_external_trigger(void);

#endif /* IQMOS_RX_H_ */
