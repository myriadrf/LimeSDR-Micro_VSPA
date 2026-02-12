/* SPDX-License-Identifier: BSD-3-Clause */

/*
 * Copyright 2024 NXP
 */

#ifndef IQMOD_TX_H_
#define IQMOD_TX_H_

#define TX_DMA_TXR_size (512)
#define TX_DDR_STEP (4 * TX_DMA_TXR_size) // to get 589 MB/s ./imx_dma -w -a 0x96400000 -d 0x1F001000  -s 8192
#define TX_UPSMP 1

#ifdef IQMOD_RX_1T0R
#define TX_NUM_BUF 8
#define TX_NUM_QEC_BUF 7
#endif

#ifdef IQMOD_RX_1T1R
#define TX_NUM_BUF 4
#define TX_NUM_QEC_BUF 3
#endif

#ifdef IQMOD_RX_1T2R
#define TX_QEC_INPLACE 1
#define TX_NUM_BUF 4
#endif

#ifdef IQMOD_RX_1T4R
#define TX_QEC_INPLACE 1
#define TX_NUM_BUF 3
#endif
#ifdef IQMOD_RX_4DDC
#define TX_QEC_INPLACE 1
#define TX_NUM_BUF 4
#endif
#ifdef IQMOD_RX_8DDC
#define TX_QEC_INPLACE 1
#define TX_NUM_BUF 4
#define TX_NUM_QEC_BUF 3
#endif

#ifdef __VSPA__

#include "txiqcomp.h"

void axiq_tx_first_initialize();

void DDR_read(uint32_t DDR_rd_dma_channel, uint32_t DDR_address, uint32_t vsp_address, int32_t bytes_size);
void tx_qec_correction(vspa_complex_fixed16 *dataIn, vspa_complex_fixed16 *dataOut);
void TX_IQ_DATA_FROM_DDR(void);
void PUSH_TX_DATA(void);
void DDR_write_VSPA_PROXY(uint32_t DDR_wr_dma_channel, uint32_t DDR_address, uint32_t vsp_address, uint32_t size);

extern uint32_t DDR_rd_start_bit_update, DDR_rd_load_start_bit_update;
extern uint32_t tx_proxy_updated;

#define DDR_RD_DMA_CHANNEL_1 0x7
#define DDR_RD_DMA_CHANNEL_2 0x8
#define DDR_RD_DMA_CHANNEL_3 0x9
#define DDR_RD_DMA_CHANNEL_4 0xa
//#define DDR_RD_DMA_CHANNEL_MASK 0x00000180
//#define DDR_RD_DMA_CHANNEL_MASK 0x00000780

#define DMA_CHANNEL_WR 0xb

#define DDR_WR_DMA_CHANNEL_5 0x0

extern vspa_complex_fixed16 output_buffer[] __attribute__((aligned(64)));
extern vspa_complex_fixed16 output_qec_buffer[] __attribute__((aligned(64)));
extern uint32_t DDR_rd_base_address;

#define INCR_TX_BUFF(txbuff_ptr)                                          \
    {                                                                     \
        /*txbuff_ptr##_prev=txbuff_ptr;*/                                 \
        txbuff_ptr += TX_DMA_TXR_size;                                    \
        if (txbuff_ptr >= &output_buffer[TX_NUM_BUF * TX_DMA_TXR_size]) { \
            txbuff_ptr = &output_buffer[0];                               \
        }                                                                 \
    }
#define INCR_TX_QEC_BUFF(txbuff_ptr)                                              \
    {                                                                             \
        /*txbuff_ptr##_prev=txbuff_ptr;*/                                         \
        txbuff_ptr += TX_DMA_TXR_size;                                            \
        if (txbuff_ptr >= &output_qec_buffer[TX_NUM_QEC_BUF * TX_DMA_TXR_size]) { \
            txbuff_ptr = &output_qec_buffer[0];                                   \
        }                                                                         \
    }

#endif

#endif /* IQMOD_TX_H_ */
