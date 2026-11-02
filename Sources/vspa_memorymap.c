#include "vspa_memorymap.h"

#include "l1-trace.h"
#include "receiver.h"
#include "transmitter.h"
#include "vspa_iqstream.h"

extern struct ADC_lane rx_adc[];
extern struct PipeStats rx_stats[];
extern struct PipeStats tx_stats;
extern struct VSPA_Stats vspa_core_stats;
extern tx_tdd_config_t tx_tdd;

// table of memory locations that can be discovered by software and interacted directly
const vspa_feature_t features_map[] __attribute__((section(".mmap_entry"))) = {
#if TRACE_ENABLED
    { VSPA_MMAP_L1_TRACE, (uint32_t)&trace_hif },
#endif
    { VSPA_MMAP_RXDMA_LANE0, (uint32_t)&rx_ddr[0].ddr.dma },
    { VSPA_MMAP_RXDDR_OUTPUT_LANE0, (uint32_t)&rx_ddr[0].output_blocks },
#if RX_MAX_LANE_COUNT > 1
    { VSPA_MMAP_RXDMA_LANE1, (uint32_t)&rx_ddr[1].ddr.dma },
    { VSPA_MMAP_RXDDR_OUTPUT_LANE1, (uint32_t)&rx_ddr[1].output_blocks },
#endif
#if RX_MAX_LANE_COUNT > 2
    { VSPA_MMAP_RXDMA_LANE2, (uint32_t)&rx_ddr[2].ddr.dma },
    { VSPA_MMAP_RXDDR_OUTPUT_LANE2, (uint32_t)&rx_ddr[2].output_blocks },
#endif
#if RX_MAX_LANE_COUNT > 3
    { VSPA_MMAP_RXDMA_LANE3, (uint32_t)&rx_ddr[3].ddr.dma },
    { VSPA_MMAP_RXDDR_OUTPUT_LANE3, (uint32_t)&rx_ddr[3].output_blocks },
#endif
    { VSPA_MMAP_TXDMA_LANE0, (uint32_t)(&(txddr[0].ddr.dma)) },
    { VSPA_MMAP_STATS, (uint32_t)&rx_stats[0] },
    { VSPA_MMAP_STATS2, (uint32_t)&tx_stats },
    { VSPA_MMAP_TX_TONE_LANE0, (uint32_t)&tx_tone_state[0] },
    { VSPA_MMAP_VSPA_CORE_STATS, (uint32_t)&vspa_core_stats },
    { VSPA_MMAP_TX_TDD_CONFIG, (uint32_t)&tx_tdd },
    { VSPA_MMAP_NONE, 0 }
};
