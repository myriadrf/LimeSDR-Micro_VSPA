/* SPDX-License-Identifier: BSD-3-Clause */

/*
 * Copyright 2024 NXP
 */

#ifndef IQMOD_TX_H_
#define IQMOD_TX_H_

#ifndef IQMOD_4DEC4INT
#define TX_DMA_TXR_size (512)
#else
#define TX_DMA_TXR_size (256)
#endif
#define TX_DDR_STEP (4 * TX_DMA_TXR_size) // to get 589 MB/s ./imx_dma -w -a 0x96400000 -d 0x1F001000  -s 8192
#if defined(IQMOD_2DEC2INT)
#define TX_UPSMP 2 // Hardcoded for 2x interpolation
#elif defined(IQMOD_4DEC4INT)
#define TX_UPSMP 4 // Hardcoded for 4x interpolation
#else
#define TX_UPSMP 1
#endif

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

#ifdef IQMOD_2DEC2INT
#define TX_NUM_BUF 4
#define TX_NUM_QEC_BUF 2
#define TX_NUM_INTERP_BUF 2
#define TX_INTERP_STEP (4 * TX_INTERP_SIZE)
#define TX_INTERP_SIZE (2 * TX_DMA_TXR_size)
#define FILTER_TAP_UPSAMPLE 32
#define NUM_HISTORY_SAMPLES (FILTER_TAP_UPSAMPLE - 1)
#endif

#ifdef IQMOD_4DEC4INT
#define TX_NUM_BUF 2
#define TX_NUM_QEC_BUF 2
#define TX_NUM_INTERP_BUF 2
#define TX_INTERP_STEP (4 * TX_INTERP_SIZE)
#define TX_INTERP_SIZE (4 * TX_DMA_TXR_size)
#define FILTER_TAP_UPSAMPLE 64
#define NUM_HISTORY_SAMPLES (FILTER_TAP_UPSAMPLE - 1)
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
#if !defined(IQMOD_2DEC2INT) && !defined(IQMOD_4DEC4INT)
void tx_qec_correction(vspa_complex_fixed16 *dataIn, vspa_complex_fixed16 *dataOut);
#else
void tx_qec_correction(vspa_complex_fixed16 *dataIn, vspa_complex_fixed16 *dataOut, uint32_t num_samples);
#endif
void TX_IQ_DATA_FROM_DDR(void);
void PUSH_TX_DATA(void);
void DDR_write_VSPA_PROXY(uint32_t DDR_wr_dma_channel, uint32_t DDR_address, uint32_t vsp_address, uint32_t size);

#ifdef IQMOD_2DEC2INT
// Interpolation function prototype (from X2_interp_tap32_filter.sx)
void X2_interp_tap32_filter(__fx16 *output, __fx16 *input, unsigned int num_samples, __fx16 *history, float *filter_taps);
#endif
#ifdef IQMOD_4DEC4INT
// Interpolation function prototype (from X4_interp_tap64_filter.sx)
void X4_interp_tap64_filter(__fx16 *output, __fx16 *input, unsigned int num_samples, __fx16 *history, float *filter_taps);
#endif

extern uint32_t DDR_rd_start_bit_update, DDR_rd_load_start_bit_update;
extern uint32_t tx_proxy_updated;

#define DDR_RD_DMA_CHANNEL_1 0x7
#define DDR_RD_DMA_CHANNEL_2 0x8
#define DDR_RD_DMA_CHANNEL_3 0x9
#define DDR_RD_DMA_CHANNEL_4 0xa
// #define DDR_RD_DMA_CHANNEL_MASK 0x00000180
// #define DDR_RD_DMA_CHANNEL_MASK 0x00000780

#define DMA_CHANNEL_WR 0xb

#define DDR_WR_DMA_CHANNEL_5 0x0

extern vspa_complex_fixed16 output_buffer[] __attribute__((aligned(64)));
extern vspa_complex_fixed16 output_qec_buffer[] __attribute__((aligned(64)));
extern vspa_complex_fixed16 interp_buffer[] __attribute__((aligned(64)));
extern vspa_complex_fixed16 history_upfilter[] __attribute__((aligned(64)));
extern uint32_t DDR_rd_base_address;

#define INCR_TX_BUFF(txbuff_ptr)                                          \
    {                                                                     \
        /*txbuff_ptr##_prev=txbuff_ptr;*/                                 \
        txbuff_ptr += TX_DMA_TXR_size;                                    \
        if (txbuff_ptr >= &output_buffer[TX_NUM_BUF * TX_DMA_TXR_size]) { \
            txbuff_ptr = &output_buffer[0];                               \
        }                                                                 \
    }

#if defined(IQMOD_2DEC2INT) || defined(IQMOD_4DEC4INT)
#define INCR_TX_QEC_BUFF(txbuff_ptr)                                             \
    {                                                                            \
        txbuff_ptr += TX_INTERP_SIZE;                                            \
        if (txbuff_ptr >= &output_qec_buffer[TX_NUM_QEC_BUF * TX_INTERP_SIZE]) { \
            txbuff_ptr = &output_qec_buffer[0];                                  \
        }                                                                        \
    }
#else
#define INCR_TX_QEC_BUFF(txbuff_ptr)                                              \
    {                                                                             \
        /*txbuff_ptr##_prev=txbuff_ptr;*/                                         \
        txbuff_ptr += TX_DMA_TXR_size;                                            \
        if (txbuff_ptr >= &output_qec_buffer[TX_NUM_QEC_BUF * TX_DMA_TXR_size]) { \
            txbuff_ptr = &output_qec_buffer[0];                                   \
        }                                                                         \
    }
#endif

#if defined(IQMOD_2DEC2INT) || defined(IQMOD_4DEC4INT)
#define INCR_TX_INTERP_BUFF(interp_ptr)                                         \
    {                                                                           \
        interp_ptr += TX_INTERP_SIZE;                                           \
        if (interp_ptr >= &interp_buffer[TX_NUM_INTERP_BUF * TX_INTERP_SIZE]) { \
            interp_ptr = &interp_buffer[0];                                     \
        }                                                                       \
    }
#endif

#endif

#endif /* IQMOD_TX_H_ */
