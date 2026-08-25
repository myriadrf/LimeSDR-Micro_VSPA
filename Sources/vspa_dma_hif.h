#ifndef LIME_VSPA_DMA_HIF_H
#define LIME_VSPA_DMA_HIF_H

// Packet flags
enum {
    PKT_HAS_TIMESTAMP = (1 << 0),
    PKT_START = (1 << 1),
    PKT_END = (1 << 2),
    PKT_IRQ = (1 << 3),
    // PKT_DMA_TCD_END = (1 << 4),
};

typedef struct VSPA_DMA_TCD {
    uint32_t addr;
    uint32_t size;
    uint32_t flags;
} vspa_dma_tcd_t;

// Directly accessable data from host for TCD submission and status readback
typedef struct VSPA_DMA_HIF {
    vspa_dma_tcd_t input_tcd;
    uint32_t tcd_done_counter; // how many transactions have been completed
    uint32_t htv_pending_flag_mask; // Host to VSPA signal that input TCD is prepared
    uint32_t vth_tcd_done_flag_mask; // VSPA to host, signal that TCD has been completed
} vspa_dma_hif_t;

#endif // LIME_VSPA_DMA_HIF_H
