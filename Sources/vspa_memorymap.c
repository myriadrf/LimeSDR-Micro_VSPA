#include "vspa_memorymap.h"

#include "l1-trace.h"
#include "receiver.h"
#include "transmitter.h"
#include "vspa_iqstream.h"
#include "fw_config.h"
#include "tdd.h"
#include "dfe.h"

extern struct ADC_lane rx_adc[];
extern struct PipeStats rx_stats[];
extern struct PipeStats tx_stats;
extern struct VSPA_Stats vspa_core_stats;
extern struct phytimer_ctrl_m4 m4_phytime_args;
extern tx_tdd_config_t tx_tdd;

const fw_config_t fw_info = {
    .rx_pipe_count = RX_PIPE_COUNT,
    .tx_pipe_count = TX_PIPE_COUNT,
    .max_decimation_exponent = MAX_DECIMATION_EXPONENT,
    .max_interpolation_exponent = MAX_INTERPOLATION_EXPONENT,
    .rx_channel_count = RX_CHANNEL_COUNT,
    .tx_channel_count = TX_CHANNEL_COUNT,
};

// table of memory locations that can be discovered by software and interacted directly
const vspa_feature_t features_map[] __attribute__((section(".mmap_entry"))) = {
#if TRACE_ENABLED
    { VSPA_MMAP_L1_TRACE, (uint32_t)&trace_hif },
#endif
    { VSPA_MMAP_TXDMA_LANE0, (uint32_t)&tx_hdma[0] },
    { VSPA_MMAP_STATS, (uint32_t)&rx_stats[0] },
    { VSPA_MMAP_STATS2, (uint32_t)&tx_stats },
    { VSPA_MMAP_TX_TONE_LANE0, (uint32_t)&tx_tone_state[0] },
    { VSPA_MMAP_VSPA_CORE_STATS, (uint32_t)&vspa_core_stats },
    { VSPA_MMAP_TX_TDD_CONFIG, (uint32_t)&tx_tdd },
    { VSPA_MMAP_FW_INFO, (uint32_t)&fw_info },
#if PHYTIMERS_THROUGH_M4
    { VSPA_MMAP_PHYTIME_SCHED, (uint32_t)&phytimer_data.m4_args },
#endif
    { VSPA_PIPE_RX_DMA_ARRAY, (uint32_t)&rx_hdma },
    { VSPA_PIPE_TX_DMA_ARRAY, (uint32_t)&tx_hdma },
    { VSPA_PIPE_RX_OUTPUT_ARRAY, (uint32_t)&rx_output_blocks },
    { VSPA_CHANNEL_RX_QEC_ARRAY, (uint32_t)&rx_qec_params },
    { VSPA_CHANNEL_TX_QEC_ARRAY, (uint32_t)&tx_qec_params },
    { VSPA_CHANNEL_RX_CONFIG_ARRAY, (uint32_t)&rx_channel_cfg },
    { VSPA_CHANNEL_TX_CONFIG_ARRAY, (uint32_t)&tx_channel_cfg },
    { VSPA_MMAP_NONE, 0 }
};
