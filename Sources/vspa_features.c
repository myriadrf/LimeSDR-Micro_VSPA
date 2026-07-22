#include "vspa_features.h"

#include "l1-trace.h"
#include "transmitter.h"

extern tx_dma_hif_t tx_dma_interface;

const feature_t features_map[] = {
#if TRACE_ENABLED
    { F_VSPA_L1_TRACE, (uint32_t)&trace_hif },
#endif
    { F_VSPA_TX_DMA, (uint32_t)&tx_dma_interface },
    { F_VSPA_NONE, 0 }
};
