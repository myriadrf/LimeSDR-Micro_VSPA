#include "vspa_memorymap.h"

#include "l1-trace.h"
#include "receiver.h"
#include "transmitter.h"

extern struct ADC_lane adc[];
extern struct DebugStats stats;
extern struct DebugStats2 stats2;

// table of memory locations that can be discovered by software and interacted directly
const vspa_feature_t features_map[] __attribute__((section(".mmap_entry"))) = {
#if TRACE_ENABLED
    { VSPA_MMAP_L1_TRACE, (uint32_t)&trace_hif },
#endif
    { VSPA_MMAP_ADC0, (uint32_t)&adc[0] },
    { VSPA_MMAP_ADC1, (uint32_t)&adc[1] },
    { VSPA_MMAP_RXDMA_LANE0, (uint32_t)&rxddr[0].rx_host_if },
    { VSPA_MMAP_TXDMA_LANE0, (uint32_t)(&(txddr[0].dma_hif)) },
    { VSPA_MMAP_STATS, (uint32_t)&stats },
    { VSPA_MMAP_STATS2, (uint32_t)&stats2 },
    { VSPA_MMAP_NONE, 0 }
};
