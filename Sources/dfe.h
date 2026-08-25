/* SPDX-License-Identifier: BSD-3-Clause */

/*
 * Copyright 2024 NXP
 */

#ifndef DFE_H_
#define DFE_H_

#include "vcpu.h"
#include <stdint.h>
#include "txiqcomp.h"

typedef enum {
    MBOX_EMPTY = 0,      // 0x0
    MBOX_IQ_CORR_FTAP0,  // 0x1
    MBOX_IQ_CORR_FTAP1,  // 0x2
    MBOX_IQ_CORR_FTAP2,  // 0x3
    MBOX_IQ_CORR_FTAP3,  // 0x4
    MBOX_IQ_CORR_FTAP4,  // 0x5
    MBOX_IQ_CORR_FTAP5,  // 0x6
    MBOX_IQ_CORR_FTAP6,  // 0x7
    MBOX_IQ_CORR_FTAP7,  // 0x8
    MBOX_IQ_CORR_FTAP8,  // 0x9
    MBOX_IQ_CORR_FTAP9,  // 0xA
    MBOX_IQ_CORR_FTAP10, // 0xB
    MBOX_IQ_CORR_FTAP11, // 0xC
    MBOX_IQ_CORR_FTAP12, // 0xD
    MBOX_IQ_CORR_DC_I,   // 0xE
    MBOX_IQ_CORR_DC_Q,   // 0xF
    MBOX_IQ_CORR_FDELAY, // 0x10
    MBOX_IQ_CORR_MAX,    // 0x11
} mbox_iq_corr_factor_e;

/**
 *  RX and TX kernel selection for QEC
 *  same kernel is used for Tx and Rx as far as no decimation is required.
 *  possible options :
 *   txiqcomp			 IQ phase and amplitude compensation
 *   txiqcomp_x32chf_5t  IQ phase and amplitude + IQ fractional delay compensation
 */
// #define TXIQCOMP2
// #define RXIQCOMP
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

void rf_update_iq_comp_params2(structTXIQCompParams2 *params_ptr, uint32_t rst, uint32_t idx, uint32_t val);
void rf_update_iq_comp_params(structTXIQCompParams *params_ptr, uint32_t rst, uint32_t idx, uint32_t val);

void rx_qec_correction(cfixed16_t *dataOut, const cfixed16_t *dataIn, uint32_t samplesCount);
void tx_qec_correction(cfixed16_t *dataOut, const cfixed16_t *dataIn, uint32_t samplesCount);

#endif /* DFE_H_ */
