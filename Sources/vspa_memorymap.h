#ifndef LIME_VSPA_MEMORYMAP_H
#define LIME_VSPA_MEMORYMAP_H

#include <stdint.h>

// Firmware's configuration memory spaces.
// Firmware directly uses raw data from these locations and does not do any validation.
// Host is fully responsible for data validity and ensuring when it can modify these values.
// (i.e don't change channel configurations while their data stream is active)
// Data processing values like filter taps and other variables can be changed during streaming,
// but multi word values migth be utilized for processing mid change, as host accesses memory in 32bit words.
// Values whose changes requires VSPA reaction should use commands.

typedef enum {
    VSPA_MMAP_NONE = 0,
    VSPA_MMAP_L1_TRACE,
    VSPA_MMAP_ADC0,
    VSPA_MMAP_ADC1,
    VSPA_MMAP_ADC2,
    VSPA_MMAP_ADC3,
    VSPA_MMAP_RXDMA_LANE0,
    VSPA_MMAP_RXDMA_LANE1,
    VSPA_MMAP_RXDMA_LANE2,
    VSPA_MMAP_RXDMA_LANE3,
    VSPA_MMAP_DAC,
    VSPA_MMAP_TXDMA_LANE0,
    VSPA_MMAP_STATS,
    VSPA_MMAP_STATS2,
    VSPA_MMAP_TX_TONE_LANE0,
    VSPA_MMAP_VSPA_CORE_STATS,
    VSPA_MMAP_TX_TDD_CONFIG,
    VSPA_MMAP_RXDDR_OUTPUT_LANE0,
    VSPA_MMAP_RXDDR_OUTPUT_LANE1,
    VSPA_MMAP_RXDDR_OUTPUT_LANE2,
    VSPA_MMAP_RXDDR_OUTPUT_LANE3,
    VSPA_MMAP_FW_INFO,
    VSPA_MMAP_PHYTIME_SCHED,
    VSPA_PIPE_RX_DMA_ARRAY,
    VSPA_PIPE_TX_DMA_ARRAY,
    VSPA_PIPE_RX_AXIQ_CONFIG_ARRAY, // unused, VSPA configures the AXIQ on channel select
    VSPA_PIPE_TX_AXIQ_CONFIG_ARRAY, // unused
    VSPA_CHANNEL_TX_QEC_ARRAY,
    VSPA_CHANNEL_RX_QEC_ARRAY,
    VSPA_CHANNEL_RX_CONFIG_ARRAY,
    VSPA_CHANNEL_TX_CONFIG_ARRAY,
    VSPA_PIPE_RX_OUTPUT_ARRAY, // dma_tcd_fifo_t per Rx pipe, completed TCDs
} e_vspa_feature;

typedef struct {
    uint32_t feature;
    uint32_t address;
} vspa_feature_t;

// IQ imbalance and DC offset correction of a channel, VSPA_CHANNEL_RX/TX_QEC_ARRAY, indexed by e_rx_channel for Rx and
// Tx channel for Tx. Written by host, used by the vspa-lib txiqcomp kernel on every buffer of whichever pipe streams the
// channel; its first 24 bytes are the kernel's structTXIQCompParams, copied to a DMEM line aligned work config before
// each use. Changes take effect on the next buffer, which may see a partially written update.
//   i' = dc_offset_i + i * ftaps[2]
//   q' = dc_offset_q + q * ftaps[3] + i * ftaps[1]
// Passthrough: dc_offset 0, ftaps {0, 0, 1, 1}.
typedef struct vspa_qec_params {
    float dc_offset_i; // added to I, 1.0 is full scale
    float dc_offset_q; // added to Q
    float ftaps[4]; // {0 (unused), ci2q, ci, cq}
    uint32_t reserved[2]; // pads to 32 bytes, the array stride host uses
} vspa_qec_params_t;
#ifdef __cplusplus
static_assert(sizeof(vspa_qec_params_t) == 32, "host indexes the VSPA array with the same stride");
#endif

// Persistent parameters of a channel, VSPA_CHANNEL_RX/TX_CONFIG_ARRAY, indexed by e_rx_channel for Rx and Tx channel for
// Tx. Used directly by whichever pipe streams the channel, so change them only while that pipe is not streaming.
typedef struct vspa_channel_config {
    uint32_t oversample_exponent : 4; // decimation (Rx) or interpolation (Tx) as power of 2, 0..max_*_exponent of fw_info
    uint32_t clock_divider_disabled : 1;
    uint32_t reserved : 27;
} vspa_channel_config_t;
#ifdef __cplusplus
static_assert(sizeof(vspa_channel_config_t) == 4, "host indexes the VSPA array with the same stride");
#endif

// Firmware build configuration (VSPA_MMAP_FW_INFO), read only
typedef struct fw_config {
    uint32_t rx_pipe_count : 4;
    uint32_t tx_pipe_count : 4;
    uint32_t max_decimation_exponent : 4;
    uint32_t max_interpolation_exponent : 4;
    uint32_t rx_channel_count : 4; // 0 in firmware older than per channel QEC
    uint32_t tx_channel_count : 4;
} fw_config_t;

#endif
