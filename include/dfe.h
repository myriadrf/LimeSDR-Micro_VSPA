/* SPDX-License-Identifier: BSD-3-Clause */

/*
 * Copyright 2024 NXP
 */

#ifndef DFE_H_
#define DFE_H_

#include "txiqcomp.h"
#include "gpio.h"
#include "chip-la9310.h"
#include "axiq-la9310.h"

/**
 *  RX and TX kernel selection for QEC
 *  same kernel is used for Tx and Rx as far as no decimation is required.
 *  possible options :
 *   txiqcomp			 IQ phase and amplitude compensation
 *   txiqcomp_x32chf_5t  IQ phase and amplitude + IQ fractional delay compensation
 */
//#define TXIQCOMP2
//#define RXIQCOMP2
#define TXIQCOMP
#define RXIQCOMP

#ifdef TXIQCOMP2
extern structTXIQCompParams2 iq_comp_params2_tx _VSPA_VECTOR_ALIGN;
#else
extern structTXIQCompParams txiqcompcfg_struct _VSPA_VECTOR_ALIGN;
#endif

#ifdef RXIQCOMP2
extern structTXIQCompParams2 iq_comp_params2_rx _VSPA_VECTOR_ALIGN;
#else
extern structTXIQCompParams rxiqcompcfg_struct _VSPA_VECTOR_ALIGN;
#endif

void rf_init(void);
void rf_update_iq_comp_params2(structTXIQCompParams2 *params_ptr, uint32_t rst, uint32_t idx, uint32_t val);
void rf_update_iq_comp_params(structTXIQCompParams *params_ptr, uint32_t rst, uint32_t idx, uint32_t val);
void stream_write_ptr_rst(uint32_t dma_channel_wr, uint32_t axi_wr);
void stream_read_ptr_rst(uint32_t dma_channel_rd, uint32_t axi_rd);
void stream_write(uint32_t dma_channel_wr, uint32_t axi_wr, uint32_t vsp);
void stream_read(uint32_t dma_channel_rd, uint32_t axi_rd, uint32_t vsp);

#endif /* DFE_H_ */
