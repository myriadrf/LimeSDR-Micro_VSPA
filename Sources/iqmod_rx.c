/* SPDX-License-Identifier: BSD-3-Clause */

/*
 * Copyright 2024 NXP
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <vspa/intrinsics.h>

#include "chip.h"
#include "vcpu.h"
#include "host.h"
#include "dmac.h"
#include "iohw.h"
#include "txiqcomp.h"
#include "dfe.h"
#include "main.h"
#include "l1-trace.h"
#include "iqmod_tx.h"
#include "cal_signal.h"
#include "iqmod_rx.h"
#include "stats.h"
#include "vspa_dmem_proxy.h"

vspa_complex_fixed16 input_buffer[RX_NUM_BUF * RX_DMA_TXR_size] __attribute__((section(".vcpu_dmem")))
__attribute__((aligned(64))) = { 0x00000000, 0x00010001, 0x00020002, 0x00030003 };
vspa_complex_fixed16 input_qec_buffer[RX_NUM_QEC_BUF * RX_DMA_TXR_size] __attribute__((section(".ippu_dmem")))
__attribute__((aligned(64))) = { 0x00000000, 0x00010001, 0x00020002, 0x00030003 };

// uint32_t DDR_wr_QEC_enable= 0, DDR_wr_CMP_enable=0;
#define DDR_wr_QEC_enable 1
#define DDR_wr_CMP_enable 0
uint32_t DDR_wr_start_bit_update = 0, DDR_wr_load_start_bit_update = 0, DDR_wr_continuous = 0;
uint32_t ddr_wr_dma_xfr_size = RX_DDR_STEP;
uint32_t DDR_wr_buff_wrap_equeued = 0;
uint32_t DDR_wr_buff_loop_count = 0;
static uint32_t host_flow_control_disable = 1;

volatile uint32_t RX_ext_dma_enabled = 0;
uint32_t ddr_wr_dma_ch_nb = 0;
uint32_t ddr_wr_dma_ch_mask = 0;

static uint32_t RX_total_axiq_enqueued_size = 0;                                 /* 0: Axiq rx in fifo cmd	*/
static uint32_t RX_total_axiq_received_size = 0;                                 /* 1: Axiq rx dma completed	*/
volatile uint32_t RX_total_dmem_QECed_size = 0;                                  /* 2: Rx data QECed			*/
#define RX_total_dmem_CMPed_size (rx_vspa_proxy[0].la9310_fifo_produced_size)    /* 4: compressed            */
static uint32_t RX_total_ddr_enqueued_size = 0;                                  /* 5: DDR wr cmd fifo 		*/
#define RX_total_dmem_consumed_size (rx_vspa_proxy[0].la9310_fifo_consumed_size) /* 6: xfer to DDR complete 	*/
#define RX_total_ddr_consumed_size (tx_vspa_proxy.host_consumed_size[0])         /* 7: DDR data ready */

static vspa_complex_fixed16 *p_rx_axiq_enqueued = &input_buffer[0];
static vspa_complex_fixed16 *p_rx_axiq_received = &input_buffer[0];
static vspa_complex_fixed16 *p_rx_dmem_QECed_in = &input_buffer[0];
static vspa_complex_fixed16 *p_rx_dmem_QECed_out = &input_qec_buffer[0];
static vspa_complex_fixed16 *p_rx_dmem_CMPed = &input_qec_buffer[0];
static vspa_complex_fixed16 *p_rx_ddr_enqueued = &input_qec_buffer[0];
static vspa_complex_fixed16 *p_rx_consumed = &input_qec_buffer[0];

// uint32_t DDR_wr_base_address = 0xdeadbeef;
uint32_t DDR_wr_offset = 0;
// uint32_t DDR_wr_size;

#define DDR_wr_base_address (rx_vspa_proxy[0].DDR_wr_base_address)
#define DDR_wr_size (rx_vspa_proxy[0].DDR_wr_size)

uint32_t dma_chan_mask(uint32_t dma_channel, uint32_t nb_dma) {
    uint32_t i, mask = 0;

    for (i = 0; i < nb_dma; i++) {
#pragma loop_count(1, 16, 2, 0)
        mask |= 1 << (dma_channel + i);
    }
    return mask;
}

void DDR_write_multi_dma(uint32_t DDR_wr_dma_channel, uint32_t nb_dma, uint32_t DDR_address, uint32_t vsp_address,
                         int32_t bytes_size) {
    uint32_t i;
    uint32_t size = bytes_size / nb_dma;

    // user should ensure bytes_size/nb_dma is enire and aligned on AXI bus width 16B.
    // 2048B is ok with 1,2,4 dmas only
    // dma channel should be contiguous

    for (i = 0; i < nb_dma; i++) {
#pragma loop_count(1, 16, 2, 0)
        uint32_t ctrl = DMAC_WRC | (DDR_wr_dma_channel + i);
        dmac_enable(ctrl, size, DDR_address + i * size, vsp_address + i * size);
    }
}

volatile cmp_cycle = 0;
#define CMP_CYCLES (24 + 2 * 512 / 32)
void rx_compress(vspa_complex_fixed16 *data) {
    if (!DDR_wr_CMP_enable)
        return;
    // compression place holder
    for (cmp_cycle = 0; cmp_cycle < CMP_CYCLES; cmp_cycle++) {
    };
}
void rx_qec_correction(vspa_complex_fixed16 *dataIn, vspa_complex_fixed16 *dataOut) {
    if (!DDR_wr_QEC_enable)
        return;

#ifdef RXIQCOMP2
    txiqcomp_x32chf_5t((vspa_complex_fixed16 *)dataIn, (vspa_complex_fixed16 *)dataOut, &iq_comp_params2_rx, MEM_LINE_SIZE);
#else
#ifdef RXIQCOMP
    txiqcomp((vspa_complex_fixed16 *)dataIn, (vspa_complex_fixed16 *)dataOut, &rxiqcompcfg_struct, MEM_LINE_SIZE);
#endif
#endif
}

//__attribute__(( section(".text.opcode_6") ))
void RX_IQ_DATA_TO_DDR(void) {
    uint64_t msg64 = host_mbox0_read();
    uint32_t cmd_start = (HIWORD(msg64)) & 0x00100000;

    if ((cmd_start) && (!DDR_wr_start_bit_update)) {
        DDR_wr_start_bit_update = 1;
        DDR_wr_load_start_bit_update = (HIWORD(msg64)) & 0x00200000;
        DDR_wr_continuous = (HIWORD(msg64)) & 0x00800000;
        // DDR_wr_QEC_enable= 				(HIWORD(msg64)) & 0x00400000;
        // DDR_wr_CMP_enable=  			(HIWORD(msg64)) & 0x00080000;
        ddr_wr_dma_ch_nb = ((HIWORD(msg64)) & 0x00070000) >> 16;
        host_flow_control_disable = (HIWORD(msg64)) & 0x00400000;

        if (!DDR_wr_continuous)
            host_flow_control_disable = 0;

        if (ddr_wr_dma_ch_nb > 2)
            goto fail_rx_iq_data;
        if (!ddr_wr_dma_ch_nb) {
            // default 1 DMA write 526MB/s half duplex, 490MB/s full duplex
            ddr_wr_dma_ch_nb = 1;
        }
        ddr_wr_dma_ch_mask = dma_chan_mask(DDR_WR_DMA_CHANNEL_1, ddr_wr_dma_ch_nb);
        ddr_wr_dma_xfr_size = (DDR_wr_CMP_enable ? RX_DDR_STEP * RX_COMPRESS_RATIO_PCT / 100 : RX_DDR_STEP);

        dmac_reset(0x1 << dma_channel_rd);

        p_rx_axiq_enqueued = &input_buffer[0];
        p_rx_axiq_received = &input_buffer[0];
        p_rx_dmem_QECed_in = &input_buffer[0];
        p_rx_dmem_QECed_out = &input_qec_buffer[0];
        p_rx_dmem_CMPed = &input_qec_buffer[0];
        p_rx_ddr_enqueued = &input_qec_buffer[0];
        p_rx_consumed = &input_qec_buffer[0];

        RX_total_axiq_enqueued_size = 0;
        RX_total_axiq_received_size = 0;
        RX_total_dmem_QECed_size = 0;
        RX_total_dmem_CMPed_size = 0;
        RX_total_ddr_enqueued_size = 0;
        RX_total_dmem_consumed_size = 0;

        DDR_wr_size = ((mailbox_in_msg_0_MSB & 0x0000FFFF) * SIZE_4K); // send chunks of 4KB can be sent
        DDR_wr_base_address = mailbox_in_msg_0_LSB;
        DDR_wr_offset = 0;

        DDR_wr_buff_wrap_equeued = 0;
        DDR_wr_buff_loop_count = 0;

        // update host vspa_dmem_proxy
        rx_proxy_updated = 1;
        tx_proxy_updated = 1; /* tx proxy contains also some rx attributes  */

        /* LA9310 AXI bus supports 2 opened WR transactions
         * VSPA DMA DDR write reach 2x DMA 529MB/s, good enough for 122.88MSPS
         *
         */

        // if simple load test no axiq init
        if (DDR_wr_load_start_bit_update != 0) {
            DDR_wr_load_start_bit_update = 1;
            // Ack mailbox
            mailbox_out_msg_0_MSB = 0;
            mailbox_out_msg_0_LSB = 0x1;
            host_mbox0_post(MAKEDWORD(mailbox_out_msg_0_MSB, mailbox_out_msg_0_LSB));
            return;
        }

        // stop single tone
        RX_SingleT_start_bit_update = 0;

        // Disable Rx and reset fifo
        axiq_fifo_rx_disable(AXIQ_BANK_0, Rx_Antenna2fifo_index[RX_index]);
        stream_read_ptr_rst(dma_channel_rd, axi_rd);
        do {
#ifndef IQMOD_RX_0T1R
            PUSH_TX_DATA();
#endif
        } while (!dmac_is_complete(0x1 << dma_channel_rd) || !dmac_errxfr(0x1 << dma_channel_rd));
        dmac_clear_complete(0x1 << dma_channel_rd);

        if (DDR_wr_continuous) {
            // ack stream start
            mailbox_out_msg_0_MSB = 0;
            mailbox_out_msg_0_LSB = 0x1;
            host_mbox0_post(MAKEDWORD(mailbox_out_msg_0_MSB, mailbox_out_msg_0_LSB));
        }

        axiq_fifo_rx_enable(AXIQ_BANK_0, Rx_Antenna2fifo_index[RX_index]);
        axiq_fifo_rx_cr(AXIQ_BANK_0, Rx_Antenna2fifo_index[RX_index], AXIQ_CR_CLRERR, AXIQ_CR_CLRERR);
        axiq_fifo_rx_cr(AXIQ_BANK_0, Rx_Antenna2fifo_index[RX_index], AXIQ_CR_CLRERR, 0);

        dmac_clear_errxfr(0x1 << dma_channel_rd);

        return;
    }

    if (cmd_start == 0) /* stop */
    {
        if (!RX_SingleT_start_bit_update) {
            dmac_abort(0x1 << dma_channel_rd);
            dmac_clear_complete(0x1 << dma_channel_rd);
            axiq_fifo_rx_disable(AXIQ_BANK_0, Rx_Antenna2fifo_index[RX_index]);
        }

        DDR_wr_base_address = 0xdeadbeef;
        RX_total_dmem_CMPed_size = 0;
        RX_total_dmem_consumed_size = 0;

        DDR_wr_start_bit_update = 0;
        DDR_wr_load_start_bit_update = 0;
        RX_SingleT_start_bit_update = 0;
        mailbox_out_msg_0_MSB = 0;
        mailbox_out_msg_0_LSB = 0x1;
        host_mbox0_post(MAKEDWORD(mailbox_out_msg_0_MSB, mailbox_out_msg_0_LSB));

        // update host vspa_dmem_proxy
        rx_proxy_updated = 1;

        return;
    }

fail_rx_iq_data:

    /* failure send Nack */
    mailbox_out_msg_0_MSB = 0;
    mailbox_out_msg_0_LSB = 0x0;
    host_mbox0_post(MAKEDWORD(mailbox_out_msg_0_MSB, mailbox_out_msg_0_LSB));
    return;
}

//#pragma optimize_for_size off
volatile uint32_t rx_busy_size = 0;
void PUSH_RX_DATA(void) {
    uint32_t tmp_status;
    uint32_t tmp_dma_errors;
    uint32_t rx_empty_size;

    // Check AXIQ rx fifo is not full or overrun
    tmp_status = axiq_fifo_rx_sr(AXIQ_BANK_0, Rx_Antenna2fifo_index[RX_index], AXIQ_SR_FIELD_ERROVER | AXIQ_SR_FIELD_ERRUNDER);
    if (tmp_status != 0) {
        if (tmp_status & (AXIQ_SR_FIELD_ERROVER << axiq_sr_shift(Rx_Antenna2fifo_index[RX_index]))) {
            g_stats.rx_stats[0][ERROR_AXIQ_FIFO_RX_OVERRUN]++;
            l1_trace(L1_TRACE_MSG_DMA_AXIQ_RX_OVER, (uint32_t)g_stats.rx_stats[0][ERROR_AXIQ_FIFO_RX_OVERRUN]);
        }
        if (tmp_status & (AXIQ_SR_FIELD_ERRUNDER << axiq_sr_shift(Rx_Antenna2fifo_index[RX_index]))) {
            g_stats.rx_stats[0][ERROR_AXIQ_FIFO_RX_UNDERRUN]++;
            l1_trace(L1_TRACE_MSG_DMA_AXIQ_RX_UNDER, (uint32_t)g_stats.rx_stats[0][ERROR_AXIQ_FIFO_RX_UNDERRUN]);
        }
        // l1_trace_disable = 1;
        axiq_fifo_rx_cr(AXIQ_BANK_0, Rx_Antenna2fifo_index[RX_index], AXIQ_CR_CLRERR, AXIQ_CR_CLRERR);
        axiq_fifo_rx_cr(AXIQ_BANK_0, Rx_Antenna2fifo_index[RX_index], AXIQ_CR_CLRERR, 0);
    }

    // DDR DMA load test , just fetch DDR as fast as possible
    if (DDR_wr_load_start_bit_update) {
        if (!RX_ext_dma_enabled) {
            // check DDR dma completion
            if (dmac_is_complete(ddr_wr_dma_ch_mask) == ddr_wr_dma_ch_mask) {
                dmac_clear_complete(ddr_wr_dma_ch_mask);
                g_stats.rx_stats[0][STAT_DMA_DDR_WR]++;
                l1_trace(L1_TRACE_MSG_DMA_DDR_WR_COMP, (uint32_t)g_stats.rx_stats[0][STAT_DMA_DDR_WR]);
            }
            // restart DDR dma if possible
            p_rx_ddr_enqueued = input_qec_buffer;
            if (dmac_is_available(ddr_wr_dma_ch_mask) == ddr_wr_dma_ch_mask) {
                DDR_write_multi_dma(DDR_WR_DMA_CHANNEL_1, ddr_wr_dma_ch_nb, DDR_wr_base_address + DDR_wr_offset,
                                    2 * (uint32_t)p_rx_ddr_enqueued, ddr_wr_dma_xfr_size);
                DDR_wr_offset = (DDR_wr_offset + ddr_wr_dma_xfr_size) % DDR_wr_size;
                l1_trace(L1_TRACE_MSG_DMA_DDR_WR_START, (uint32_t)p_rx_ddr_enqueued);
            }
        }
    }

#ifndef IQMOD_RX_0T1R
    // Single Tone
    if (RX_SingleT_start_bit_update) {
        // check axiq dma completion (received new samples) and QEC them
        if (dmac_is_complete(0x1 << dma_channel_rd)) {
            dmac_clear_complete(0x1 << dma_channel_rd);
            dmac_clear_event(0x1 << dma_channel_rd);
            g_stats.rx_stats[0][STAT_DMA_AXIQ_READ]++;
            l1_trace(L1_TRACE_MSG_DMA_AXIQ_RX_COMP, (uint32_t)g_stats.rx_stats[0][STAT_DMA_AXIQ_READ]);
        }
        // restart axiq  dma if possible
        if (dmac_is_available(0x1 << dma_channel_rd)) {
            stream_read(dma_channel_rd, axi_rd, 2 * (uint32_t)(RX_SingleT_buffer));
            l1_trace(L1_TRACE_MSG_DMA_AXIQ_RX_START, (uint32_t)RX_SingleT_buffer);
        }
    }
#endif

    // if any of previous load test is on, skip DDR streaming
    if (DDR_wr_load_start_bit_update || RX_SingleT_start_bit_update)
        goto end_rx_push;

    // Stream waveform to DDR
    if (DDR_wr_start_bit_update) {
        // check axiq dma completion (received new samples) and QEC them
        if (dmac_is_complete(0x1 << dma_channel_rd)) {
            dmac_clear_complete(0x1 << dma_channel_rd);
            dmac_clear_event(0x1 << dma_channel_rd);
            RX_total_axiq_received_size += RX_DDR_STEP;
            g_stats.rx_stats[0][STAT_DMA_AXIQ_READ]++;
            l1_trace(L1_TRACE_MSG_DMA_AXIQ_RX_COMP, (uint32_t)g_stats.rx_stats[0][STAT_DMA_AXIQ_READ]);
            // check axiq dma error
            if (dmac_errxfr(0x1 << dma_channel_rd)) {
                dmac_clear_errxfr(0x1 << dma_channel_rd);
                g_stats.gbl_stats[ERROR_DMA_XFER_ERROR]++;
                l1_trace(L1_TRACE_MSG_DMA_AXIQ_RX_XFER_ERROR, (uint32_t)g_stats.gbl_stats[ERROR_DMA_XFER_ERROR]);
            }
        }
        // restart axiq  dma if possible
        if (dmac_is_available(0x1 << dma_channel_rd)) {
            rx_busy_size = RX_total_axiq_enqueued_size - RX_total_dmem_QECed_size;
            rx_empty_size = (RX_NUM_BUF * RX_DDR_STEP) - rx_busy_size;
            if (rx_empty_size >= RX_DDR_STEP) {
                stream_read(dma_channel_rd, axi_rd, 2 * (uint32_t)(p_rx_axiq_enqueued));
                INCR_RX_BUFF(p_rx_axiq_enqueued);
                RX_total_axiq_enqueued_size += RX_DDR_STEP;
                l1_trace(L1_TRACE_MSG_DMA_AXIQ_RX_START, (uint32_t)p_rx_axiq_enqueued);
            } else {
                // overflow no more dmem buffer to arm new axiq DMA
                g_stats.rx_stats[0][ERROR_DMA_DDR_WR_OVERRUN]++;
                l1_trace(L1_TRACE_MSG_DMA_DDR_WR_OVERRUN, (uint32_t)g_stats.rx_stats[0][ERROR_DMA_DDR_WR_OVERRUN]);
                // l1_trace_disable = 1;
            }
        }
        if ((RX_total_axiq_received_size - RX_total_dmem_QECed_size) >= RX_DDR_STEP) {
            rx_busy_size = RX_total_dmem_QECed_size - RX_total_dmem_consumed_size;
            rx_empty_size = (RX_NUM_QEC_BUF * RX_DDR_STEP) - rx_busy_size;
            if (rx_empty_size >= RX_DDR_STEP) {
                // QEC buffer just received
                l1_trace(L1_TRACE_L1APP_RX_QEC_START, (uint32_t)p_rx_dmem_QECed_in);
                rx_qec_correction((vspa_complex_fixed16 *)p_rx_dmem_QECed_in, (vspa_complex_fixed16 *)p_rx_dmem_QECed_out);
                INCR_RX_BUFF(p_rx_dmem_QECed_in);
                INCR_RX_QEC_BUFF(p_rx_dmem_QECed_out);
                RX_total_dmem_QECed_size += RX_DDR_STEP;
                l1_trace(L1_TRACE_L1APP_RX_QEC_COMP, (uint32_t)RX_total_dmem_QECed_size);
            }
        }
        if ((RX_total_dmem_QECed_size - RX_total_dmem_CMPed_size) >= RX_DDR_STEP) {
            // Compress buffer
            l1_trace(L1_TRACE_L1APP_RX_CMP_START, (uint32_t)p_rx_dmem_CMPed);
            // rx_compress((vspa_complex_fixed16 *)p_rx_dmem_CMPed);
            INCR_RX_QEC_BUFF(p_rx_dmem_CMPed);
            RX_total_dmem_CMPed_size += RX_DDR_STEP;
            l1_trace(L1_TRACE_L1APP_RX_CMP_COMP, (uint32_t)RX_total_dmem_QECed_size);

            // update host vspa_dmem_proxy
            rx_proxy_updated = 1;
        }
        // Push data to DDR if standalone mode
        if (!RX_ext_dma_enabled) {
            // check DDR dma completion
            if (dmac_is_complete(ddr_wr_dma_ch_mask) == ddr_wr_dma_ch_mask) {
                dmac_clear_complete(ddr_wr_dma_ch_mask);
                RX_total_dmem_consumed_size += RX_DDR_STEP;
                g_stats.rx_stats[0][STAT_DMA_DDR_WR]++;
                l1_trace(L1_TRACE_MSG_DMA_DDR_WR_COMP, (uint32_t)g_stats.rx_stats[0][STAT_DMA_DDR_WR]);
                if (DDR_wr_buff_wrap_equeued) {
                    DDR_wr_buff_wrap_equeued = 0;
                    DDR_wr_buff_loop_count++;
                }
            }

            // host flow control
            if ((RX_total_ddr_enqueued_size - tx_vspa_proxy.host_consumed_size[0] < DDR_wr_size) || host_flow_control_disable) {
                if ((RX_total_dmem_CMPed_size - RX_total_ddr_enqueued_size) >= RX_DDR_STEP) {
                    if (dmac_is_available(ddr_wr_dma_ch_mask) == ddr_wr_dma_ch_mask) {
                        DDR_write_multi_dma(DDR_WR_DMA_CHANNEL_1, ddr_wr_dma_ch_nb, DDR_wr_base_address + DDR_wr_offset,
                                            2 * (uint32_t)p_rx_ddr_enqueued, ddr_wr_dma_xfr_size);
                        INCR_RX_QEC_BUFF(p_rx_ddr_enqueued);
                        RX_total_ddr_enqueued_size += RX_DDR_STEP;
                        DDR_wr_offset += ddr_wr_dma_xfr_size;
                        if (DDR_wr_offset >= DDR_wr_size) {
                            DDR_wr_buff_wrap_equeued = 1;
                            DDR_wr_offset = 0;
                        }
                        l1_trace(L1_TRACE_MSG_DMA_DDR_WR_START, (uint32_t)p_rx_ddr_enqueued);
                    }
                }
            }
        }
        if ((DDR_wr_buff_loop_count) && (!DDR_wr_continuous)) {

            dmac_abort(0x1 << dma_channel_rd);
            dmac_clear_complete(0x1 << dma_channel_rd);
            axiq_fifo_rx_disable(AXIQ_BANK_0, Rx_Antenna2fifo_index[RX_index]);

            DDR_wr_base_address = 0xdeadbeef;
            DDR_wr_start_bit_update = 0;
            mailbox_out_msg_0_MSB = 0;
            mailbox_out_msg_0_LSB = 0x1;
            host_mbox0_post(MAKEDWORD(mailbox_out_msg_0_MSB, mailbox_out_msg_0_LSB));

            // update host vspa_dmem_proxy
            rx_proxy_updated = 1;
        }
    }

end_rx_push:
    /* Check DMA errors*/
    tmp_dma_errors = (uint32_t)iord(DMA_CFGERR_STAT);
    if (tmp_dma_errors != 0) {
        l1_trace(L1_TRACE_MSG_DMA_CFGERR, (uint32_t)(g_stats.gbl_stats[ERROR_DMA_CONFIG_ERROR]));
        g_stats.gbl_stats[ERROR_DMA_CONFIG_ERROR]++;
        dmac_clear_errcfg();
        while (dbg_gbl == 6) {
        };
    };

    // update host proxy if needed
    VSPA_PROXY_update();
}
#pragma optimize_for_size reset
