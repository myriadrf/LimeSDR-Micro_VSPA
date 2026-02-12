/* SPDX-License-Identifier: BSD-3-Clause */

/*
 * Copyright 2024 NXP
 */

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <vspa/intrinsics.h>

#include "chip.h"
#include "vcpu.h"
#include "host.h"
#include "dmac.h"
#include "txiqcomp.h"
#include "iqmod_rx.h"
#include "iqmod_tx.h"
#include "main.h"
#include "dfe.h"
#include "l1-trace.h"

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
    iq_comp_params2_tx.inpCircBuffBase = (vspa_complex_fixed16 *)output_buffer;
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
    iq_comp_params2_rx.inpCircBuffBase = (vspa_complex_fixed16 *)input_buffer;
    iq_comp_params2_rx.inpCircBuffSize = 4 * FFT_SIZE * 2; // sizeof(input_buffer)
#endif
}

void rf_init(void) {

    dmac_reset();
    rf_iq_comp_params_init();
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

#ifndef IQMOD_RX_0T1R
void stream_write_ptr_rst(uint32_t dma_channel_wr, uint32_t axi_wr) {
    uint32_t ctrl = DMAC_FIFO_RESET | DMAC_WRC | dma_channel_wr;
    uint32_t vsp = 2 * (uint32_t)(output_buffer);

    dmac_enable(ctrl, TX_DMA_TXR_size << 2, axi_wr, vsp);
}

void stream_write(uint32_t dma_channel_wr, uint32_t axi_wr, uint32_t vsp) {
    // convert from 2c out of the ADC/DAC to SM for local VSPA work
    // uint32_t ctrl = DMAC_FIFO | DMAC_TRIG_VCPU | DMAC_WRC | dma_channel_wr;
    uint32_t ctrl = DMAC_FIFO | DMAC_WRC | dma_channel_wr;
    // l1_trace(L1_TRACE_MSG_DMA_AXIQ_TX,(uint32_t)(dma_channel_wr<<24) +(uint32_t)vsp);
    dmac_enable(ctrl, TX_DMA_TXR_size << 2, axi_wr, vsp);
}
#endif

#ifndef IQMOD_RX_1T0R
void stream_read_ptr_rst(uint32_t dma_channel_rd, uint32_t axi_rd) {
    uint32_t ctrl = DMAC_FIFO_RESET | DMAC_RD | DMAC_WRC | dma_channel_rd;
    uint32_t vsp = 2 * (uint32_t)(input_buffer);

    dmac_enable(ctrl, RX_DMA_TXR_size << 2, axi_rd, vsp);
}

void stream_read(uint32_t dma_channel_rd, uint32_t axi_rd, uint32_t vsp) {
    // convert from 2c out of the ADC/DAC to SM for local VSPA work
    // uint32_t ctrl = DMAC_RDC | DMAC_FIFO | DMAC_TRIG_VCPU | dma_channel_rd;
    uint32_t ctrl = DMAC_RDC | DMAC_FIFO | dma_channel_rd;
    // l1_trace(L1_TRACE_MSG_DMA_AXIQ_RX,(uint32_t)(dma_channel_rd<<24) +(uint32_t)vsp);
    dmac_enable(ctrl, RX_DMA_TXR_size << 2, axi_rd, vsp);
}
#endif
