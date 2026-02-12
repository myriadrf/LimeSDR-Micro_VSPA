/* SPDX-License-Identifier: BSD-3-Clause */

/*
 * Copyright 2024 NXP
 */

#ifndef __MAIN_H__
#define __MAIN_H__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <vspa/intrinsics.h>

#include "gpio.h"
#include "iohw.h"
#include "dmac.h"
#include "axiq.h"
//#include "bitRev.h"
//#include "la9310.h"

/* parameters */

#define FFT_SIZE (512)
#define SIZE_4K (4096)
#define KILO_SIZE (1024)
#define MEM_LINE_SIZE (512 / 32)

/* global varibles - to changed by routing args instead  */

extern uint32_t Rx_Antenna2axiq_dma_chan[];
extern enum axiq_fifo_e Rx_Antenna2fifo_index[];
extern uint32_t axi_ADC_FIFO_addr[];

extern uint32_t dma_comp;
extern uint32_t axi_wr, dma_channel_rd, axi_rd;
extern uint32_t mailbox_in_0_status, mailbox_in_msg_0_MSB, mailbox_in_msg_0_LSB;
extern uint32_t mailbox_in_msg_1_MSB, mailbox_in_msg_1_LSB;
extern uint32_t fft_bin, RX_index, BW_index, DCOC_direction, tone_freq_DAC, tone_phase_DAC;
extern vspa_complex_float32 gain;
extern vspa_complex_float32 *pgain;
extern tone_index, image_tone_index;
extern volatile int dbg_gbl;
extern uint32_t TX_SingleT_start_bit_update, RX_SingleT_start_bit_update, RX_SingleT_continue;
extern uint32_t g_iqflood_proxy_offset;

extern volatile uint32_t mailbox_out_msg_0_MSB; // (VCPU_OUT_0_MSB)
extern volatile uint32_t mailbox_out_msg_0_LSB; // (VCPU_OUT_0_LSB)
extern volatile uint32_t mailbox_out_msg_1_MSB; // (VCPU_OUT_1_MSB)
extern volatile uint32_t mailbox_out_msg_1_LSB; // (VCPU_OUT_1_LSB)

typedef enum {
    MBOX_RX, // 0x0
    MBOX_TX  // 0x1
} mbox_tx_rx_e;

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

typedef enum {
    MBOX_OPC_EMPTY_0,         // 0x0
    MBOX_OPC_SINGLE_TONE_TX,  // 0x1
    MBOX_OPC_SINGLE_TONE_RX,  // 0x2
    MBOX_OPC_DCOC,            // 0x3
    MBOX_OPC_BW_CAL,          // 0x4
    MBOX_OPC_IQ_MOD_TX,       // 0x5
    MBOX_OPC_IQ_MOD_RX,       // 0x6
    MBOX_OPC_MSI,             // 0x7
    MBOX_OPC_IQ_CORR,         // 0x8
    MBOX_OPC_EMPTY_1,         // 0x9
    MBOX_OPC_EMPTY_2,         // 0xA
    MBOX_OPC_TX_DCO_CORR,     // 0xB
    MBOX_OPC_OVERLAY_BASE,    // 0xC
    MBOX_OPC_RX_CHAN_SELECT,  // 0xD
    MBOX_OPC_RX_DCO_CORR,     // 0xE
    MBOX_OPC_GET_STATS_COUNT, // 0xF
    MBOX_OPC_DONE_SWRESET,    // 0x10
    MBOX_OPC_PROXY_OFFSET     // 0x11
   
} mbox_opc_e;

void PUSH_TX_DATA(void);
void wait_for_pending_transfers(uint32_t ch);
void DDR_write_VSPA_PROXY(uint32_t DDR_wr_dma_channel, uint32_t DDR_address, uint32_t vsp_address, uint32_t size);
void VSPA_PROXY_update(void);

#endif // __MAIN_H__
