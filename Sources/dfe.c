/* SPDX-License-Identifier: BSD-3-Clause */

/*
 * Copyright 2024 NXP
 */

#include "dfe.h"

#include "txiqcomp.h"
#include "dma_common.h"

#ifdef TXIQCOMP2
structTXIQCompParams2 iq_comp_params2_tx _VSPA_VECTOR_ALIGN;
#else
structTXIQCompParams txiqcompcfg_struct _VSPA_VECTOR_ALIGN;
#endif

#ifdef RXIQCOMP2
structTXIQCompParams2 iq_comp_params2_rx _VSPA_VECTOR_ALIGN;
#else
structTXIQCompParams rxiqcompcfg_struct _VSPA_VECTOR_ALIGN;
#endif

void rf_iq_comp_params_init(void) {
#ifndef TXIQCOMP2
    // IQImb_ftaps = [0 f2 f1 f4];
    // to ignore static IQ imbalance (make it full passthrough): f1=f4=1; f2=0;
    txiqcompcfg_struct.dcOffset.real = 0.0;
    txiqcompcfg_struct.dcOffset.imag = 0.0;
    txiqcompcfg_struct.IQImb_ftaps[0] = 0.0;
    txiqcompcfg_struct.IQImb_ftaps[1] = 0.0;
    txiqcompcfg_struct.IQImb_ftaps[2] = 1.0;
    txiqcompcfg_struct.IQImb_ftaps[3] = 1.0;
#else
    // IQImb_ftaps = [0 f2 0 0 0 0 0 0 0 0 f1 f4];
    // to ignore timing skew: IQImb_delay = 1;
    // to ignore static IQ imbalance (make it full passthrough): f1=f4=1; f2=0;
    iq_comp_params2_tx.dcOffset.real = 0.0;
    iq_comp_params2_tx.dcOffset.imag = 0.0;
    iq_comp_params2_tx.IQImb_ftaps[0] = 0.0;
    iq_comp_params2_tx.IQImb_ftaps[1] = 0.0;
    iq_comp_params2_tx.IQImb_ftaps[2] = 0.0;
    iq_comp_params2_tx.IQImb_ftaps[3] = 0.0;
    iq_comp_params2_tx.IQImb_ftaps[4] = 0.0;
    iq_comp_params2_tx.IQImb_ftaps[5] = 0.0;
    iq_comp_params2_tx.IQImb_ftaps[6] = 0.0;
    iq_comp_params2_tx.IQImb_ftaps[7] = 0.0;
    iq_comp_params2_tx.IQImb_ftaps[8] = 0.0;
    iq_comp_params2_tx.IQImb_ftaps[9] = 0.0;
    iq_comp_params2_tx.IQImb_ftaps[10] = 1.0;
    iq_comp_params2_tx.IQImb_ftaps[11] = 1.0;
    iq_comp_params2_tx.IQImb_delay = 1;
    iq_comp_params2_tx.inpCircBuffBase = (cfixed16_t *)output_buffer;
    iq_comp_params2_tx.inpCircBuffSize = 3 * FFT_SIZE * 2; // sizeof(output_buffer)

#endif

#ifndef RXIQCOMP2
    // IQImb_ftaps = [0 f2 f1 f4];
    // to ignore static IQ imbalance (make it full passthrough): f1=f4=1; f2=0;
    rxiqcompcfg_struct.dcOffset.real = 0.0;
    rxiqcompcfg_struct.dcOffset.imag = 0.0;
    rxiqcompcfg_struct.IQImb_ftaps[0] = 0.0;
    rxiqcompcfg_struct.IQImb_ftaps[1] = 0.0;
    rxiqcompcfg_struct.IQImb_ftaps[2] = 1.0;
    rxiqcompcfg_struct.IQImb_ftaps[3] = 1.0;
#else
    // IQImb_ftaps = [0 f2 0 0 0 0 0 0 0 0 f1 f4];
    // to ignore timing skew: IQImb_delay = 1;
    // to ignore static IQ imbalance (make it full passthrough): f1=f4=1; f2=0;
    iq_comp_params2_rx.dcOffset.real = 0.0;
    iq_comp_params2_rx.dcOffset.imag = 0.0;
    iq_comp_params2_rx.IQImb_ftaps[0] = 0.0;
    iq_comp_params2_rx.IQImb_ftaps[1] = 0.0;
    iq_comp_params2_rx.IQImb_ftaps[2] = 0.0;
    iq_comp_params2_rx.IQImb_ftaps[3] = 0.0;
    iq_comp_params2_rx.IQImb_ftaps[4] = 0.0;
    iq_comp_params2_rx.IQImb_ftaps[5] = 0.0;
    iq_comp_params2_rx.IQImb_ftaps[6] = 0.0;
    iq_comp_params2_rx.IQImb_ftaps[7] = 0.0;
    iq_comp_params2_rx.IQImb_ftaps[8] = 0.0;
    iq_comp_params2_rx.IQImb_ftaps[9] = 0.0;
    iq_comp_params2_rx.IQImb_ftaps[10] = 1.0;
    iq_comp_params2_rx.IQImb_ftaps[11] = 1.0;
    iq_comp_params2_rx.IQImb_delay = 1;
    iq_comp_params2_rx.inpCircBuffBase = (cfixed16_t *)NULL; // input_buffer;
    iq_comp_params2_rx.inpCircBuffSize = 4 * FFT_SIZE * 2; // sizeof(input_buffer)
#endif
}

void rf_update_iq_comp_params2(structTXIQCompParams2 *params_ptr, uint32_t rst, uint32_t idx, uint32_t val) {
    if (rst) {
        // Reset params
        rf_iq_comp_params_init();
    } else {
        // Update value based on factor index
        switch (idx) {
        case MBOX_IQ_CORR_FTAP1:
        case MBOX_IQ_CORR_FTAP2:
        case MBOX_IQ_CORR_FTAP3:
        case MBOX_IQ_CORR_FTAP4:
        case MBOX_IQ_CORR_FTAP5:
        case MBOX_IQ_CORR_FTAP6:
        case MBOX_IQ_CORR_FTAP7:
        case MBOX_IQ_CORR_FTAP8:
        case MBOX_IQ_CORR_FTAP9:
        case MBOX_IQ_CORR_FTAP10:
        case MBOX_IQ_CORR_FTAP11:
        case MBOX_IQ_CORR_FTAP12:
            params_ptr->IQImb_ftaps[idx - MBOX_IQ_CORR_FTAP0] = *((float *)&val);
            break;
        case MBOX_IQ_CORR_DC_I:
            params_ptr->dcOffset.real = *((float *)&val);
            break;
        case MBOX_IQ_CORR_DC_Q:
            params_ptr->dcOffset.imag = *((float *)&val);
            break;
        case MBOX_IQ_CORR_FDELAY:
            params_ptr->IQImb_delay = val;
            break;
        default:
            break;
        }
    }
}

void rf_update_iq_comp_params(structTXIQCompParams *params_ptr, uint32_t rst, uint32_t idx, uint32_t val) {
    if (rst != 0) {
        // Reset params
        params_ptr->dcOffset.real = 0.0;
        params_ptr->dcOffset.imag = 0.0;
        params_ptr->IQImb_ftaps[0] = 0.0;
        params_ptr->IQImb_ftaps[1] = 0.0;
        params_ptr->IQImb_ftaps[2] = 1.0;
        params_ptr->IQImb_ftaps[3] = 1.0;
    } else {
        if (idx <= MBOX_IQ_CORR_FTAP3)
            params_ptr->IQImb_ftaps[idx - 1] = *((float *)&val);
        else if (idx == MBOX_IQ_CORR_DC_I)
            params_ptr->dcOffset.real = *((float *)&val);
        else if (idx == MBOX_IQ_CORR_DC_Q)
            params_ptr->dcOffset.imag = *((float *)&val);
    }
}

void rx_qec_correction(const cfixed16_t *dataIn, cfixed16_t *dataOut, uint32_t samplesCount) {
    const uint32_t dmem_lines_count = 4 * samplesCount / DMEM_LINE_SIZE_BYTES;
#ifdef RXIQCOMP2
    txiqcomp_x32chf_5t((vspa_complex_fixed16 *)dataIn, (vspa_complex_fixed16 *)dataOut, &iq_comp_params2_rx, dmem_lines_count);
#else
#ifdef RXIQCOMP
    txiqcomp((vspa_complex_fixed16 *)dataIn, (vspa_complex_fixed16 *)dataOut, &rxiqcompcfg_struct, dmem_lines_count);
#endif
#endif
}

void tx_qec_correction(const cfixed16_t *dataIn, cfixed16_t *dataOut, uint32_t samplesCount) {
    const uint32_t dmem_lines_count = 4 * samplesCount / DMEM_LINE_SIZE_BYTES;
#ifdef TXIQCOMP2
    txiqcomp_x32chf_5t((vspa_complex_fixed16 *)dataIn, (vspa_complex_fixed16 *)dataOut, &iq_comp_params2_tx, dmem_lines_count);
#endif
#ifdef TXIQCOMP
    txiqcomp((vspa_complex_fixed16 *)dataIn, (vspa_complex_fixed16 *)dataOut, &txiqcompcfg_struct, dmem_lines_count);
#endif
}
