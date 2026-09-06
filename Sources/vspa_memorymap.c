#include "vspa_memorymap.h"

#include "l1-trace.h"
#include "receiver.h"
#include "transmitter.h"
#include "vspa_iqstream.h"

extern struct ADC_lane adc[];
extern struct PipeStats rx_stats[];
extern struct PipeStats tx_stats;

// table of memory locations that can be discovered by software and interacted directly
const vspa_feature_t features_map[] __attribute__((section(".mmap_entry"))) = {
#if TRACE_ENABLED
    { VSPA_MMAP_L1_TRACE, (uint32_t)&trace_hif },
#endif
    { VSPA_MMAP_RXDMA_LANE0, (uint32_t)&rxddr[0].dma },
    { VSPA_MMAP_TXDMA_LANE0, (uint32_t)(&(txddr[0].dma_hif)) },
    { VSPA_MMAP_STATS, (uint32_t)&rx_stats[0] },
    { VSPA_MMAP_STATS2, (uint32_t)&tx_stats },
    { VSPA_MMAP_NONE, 0 }
};
