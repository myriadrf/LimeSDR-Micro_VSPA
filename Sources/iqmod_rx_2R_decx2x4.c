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
#include "main.h"
#include "stats.h"
#include "ddc2x4x.h"
#include "vspa_dmem_proxy.h"

vspa_complex_fixed16 input_buffer[RX_NUM_CHAN][RX_NUM_BUF * RX_DMA_TXR_size] __attribute__((section(".ippu_dmem")))
__attribute__((aligned(64)));
vspa_complex_fixed16 input_dec_buffer[RX_NUM_CHAN][RX_NUM_DEC_BUF * (RX_DMA_TXR_size / RX_DECIM)]
    __attribute__((section(".vcpu_dmem"))) __attribute__((aligned(64)));

cfixed16_t filtState[RX_NUM_CHAN][32] __attribute__((section(".ippu_dmem"))) __attribute__((aligned(64)));
int filter_taps_downsampling[8] __attribute__((aligned(64))) = {
#include "para_files\2xdown_coeff.txt"
};

t_rx_ch_context rx_ch_context[RX_NUM_CHAN];

// uint32_t DDR_wr_QEC_enable= 0, DDR_wr_CMP_enable=0;
#define DDR_wr_QEC_enable 1
#define DDR_wr_CMP_enable 0
uint32_t DDR_wr_start_bit_update = 0, DDR_wr_load_start_bit_update = 0, DDR_wr_continuous = 0;
volatile uint32_t RX_ext_dma_enabled = 0;
uint32_t ddr_wr_dma_ch_nb = 0;
uint32_t ddr_wr_dma_ch_mask = 0;
static uint32_t host_flow_control_disable = 1;

uint32_t ddr_wr_dma_xfr_size = RX_DDR_STEP;

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

void rx_qec_correction(vspa_complex_fixed16 *dataIn, vspa_complex_fixed16 *dataOut) {
    //	if(!DDR_wr_QEC_enable)
    //		return;

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
    uint32_t DDR_wr_size;
    int i;

    if ((cmd_start) && (!DDR_wr_start_bit_update)) {
        DDR_wr_start_bit_update = 1;
        DDR_wr_load_start_bit_update = (HIWORD(msg64)) & 0x00200000;
        DDR_wr_continuous = (HIWORD(msg64)) & 0x00800000;
        //	    DDR_wr_QEC_enable= (HIWORD(msg64)) & 0x00400000;
        //	    DDR_wr_CMP_enable =  			(HIWORD(msg64)) & 0x00080000;
        ddr_wr_dma_ch_nb = ((HIWORD(msg64)) & 0x00070000) >> 16;
        host_flow_control_disable = (HIWORD(msg64)) & 0x00400000;

        if (!DDR_wr_continuous)
            host_flow_control_disable = 0;

        if (ddr_wr_dma_ch_nb > 1)
            goto fail_rx_iq_data;
        if (!ddr_wr_dma_ch_nb) {
            // default 1 DMA write 526MB/s half duplex, 490MB/s full duplex
            ddr_wr_dma_ch_nb = 1;
        }
        DDR_wr_size = ((mailbox_in_msg_0_MSB & 0x0000FFFF) * SIZE_4K); // send chunks of 4KB can be sent
        ddr_wr_dma_xfr_size = RX_DDR_STEP;

        for (i = 0; i < RX_NUM_CHAN; i++) {
            rx_ch_context[i].RX_index = RX_index + i;
            rx_ch_context[i].p_rx_axiq_enqueued = &input_buffer[i][0];
            rx_ch_context[i].p_rx_axiq_received = &input_buffer[i][0];
            rx_ch_context[i].p_rx_dmem_QECed = &input_buffer[i][0];
            rx_ch_context[i].p_rx_dmem_input_decimated = &input_buffer[i][0];
            rx_ch_context[i].p_rx_dmem_output_decimated = &input_dec_buffer[i][0];
            rx_ch_context[i].p_rx_ddr_enqueued = &input_dec_buffer[i][0];
            rx_ch_context[i].p_rx_consumed = &input_dec_buffer[i][0];
            rx_ch_context[i].RX_total_axiq_enqueued_size = 0;
            rx_ch_context[i].RX_total_axiq_received_size = 0;
            rx_ch_context[i].RX_total_dmem_QECed_size = 0;
            rx_ch_context[i].RX_total_dmem_input_Decimated_size = 0;
            // rx_ch_context[i].RX_total_dmem_output_Decimated_size = 0;
            rx_vspa_proxy[i].la9310_fifo_produced_size = 0; // RX_total_dmem_output_Decimated_size = 0;
            rx_ch_context[i].RX_total_ddr_enqueued_size = 0;
            rx_vspa_proxy[i].la9310_fifo_consumed_size = 0;
            rx_ch_context[i].DDR_wr_offset = 0;
            // rx_ch_context[i].DDR_wr_base_address=mailbox_in_msg_0_LSB+i*DDR_wr_size/RX_NUM_CHAN;
            rx_vspa_proxy[i].DDR_wr_base_address = mailbox_in_msg_0_LSB + i * DDR_wr_size / RX_NUM_CHAN;
            rx_vspa_proxy[i].DDR_wr_size = DDR_wr_size / RX_NUM_CHAN;
        }

        // update host vspa_dmem_proxy
        rx_proxy_updated = 1;

        // clear decimation history buffers
        memclr((void *)filtState, sizeof(filtState));

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
        for (i = 0; i < RX_NUM_CHAN; i++) {
            dma_channel_rd = Rx_Antenna2axiq_dma_chan[i + RX_index];
            dmac_reset(0x1 << dma_channel_rd);
            axiq_fifo_rx_disable(AXIQ_BANK_0, Rx_Antenna2fifo_index[rx_ch_context[i].RX_index]);
            stream_read_ptr_rst(dma_channel_rd, axi_rd);
            do {
                PUSH_TX_DATA();
            } while (!dmac_is_complete(0x1 << dma_channel_rd) || !dmac_errxfr(0x1 << dma_channel_rd));
            dmac_clear_complete(0x1 << dma_channel_rd);
        }

        if (DDR_wr_continuous) {
            // ack stream start
            mailbox_out_msg_0_MSB = 0;
            mailbox_out_msg_0_LSB = 0x1;
            host_mbox0_post(MAKEDWORD(mailbox_out_msg_0_MSB, mailbox_out_msg_0_LSB));
        }

        for (i = 0; i < RX_NUM_CHAN; i++) {
            dma_channel_rd = Rx_Antenna2axiq_dma_chan[i + RX_index];
            axiq_fifo_rx_enable(AXIQ_BANK_0, Rx_Antenna2fifo_index[rx_ch_context[i].RX_index]);
            axiq_fifo_rx_cr(AXIQ_BANK_0, Rx_Antenna2fifo_index[rx_ch_context[i].RX_index], AXIQ_CR_CLRERR, AXIQ_CR_CLRERR);
            axiq_fifo_rx_cr(AXIQ_BANK_0, Rx_Antenna2fifo_index[rx_ch_context[i].RX_index], AXIQ_CR_CLRERR, 0);
            dmac_clear_errxfr(0x1 << dma_channel_rd);
        }
        return;
    }

    if (cmd_start == 0) /* stop */
    {
        DDR_wr_start_bit_update = 0;
        DDR_wr_load_start_bit_update = 0;
        RX_SingleT_start_bit_update = 0;
        for (i = 0; i < RX_NUM_CHAN; i++) {
            rx_vspa_proxy[i].DDR_wr_base_address = 0xdeadbeef;
            rx_vspa_proxy[i].la9310_fifo_produced_size = 0;
            rx_vspa_proxy[i].la9310_fifo_consumed_size = 0;
            dma_channel_rd = Rx_Antenna2axiq_dma_chan[i + RX_index];
            dmac_abort(0x1 << dma_channel_rd);
            dmac_clear_complete(0x1 << dma_channel_rd);
            axiq_fifo_rx_disable(AXIQ_BANK_0, Rx_Antenna2fifo_index[rx_ch_context[i].RX_index]);
        }

        rx_proxy_updated = 1;
        mailbox_out_msg_0_MSB = 0;
        mailbox_out_msg_0_LSB = 0x1;
        host_mbox0_post(MAKEDWORD(mailbox_out_msg_0_MSB, mailbox_out_msg_0_LSB));

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
    uint32_t tmp_status, i;
    uint32_t tmp_dma_errors;
    uint32_t rx_empty_size;
    uint32_t all_chan_ddr_consumed_size = 0;

    // Check AXIQ rx fifo is not full or overrun
    for (i = 0; i < RX_NUM_CHAN; i++) {
        dma_channel_rd = Rx_Antenna2axiq_dma_chan[i + RX_index];
        axi_rd = axi_ADC_FIFO_addr[i];
        tmp_status = axiq_fifo_rx_sr(AXIQ_BANK_0, Rx_Antenna2fifo_index[rx_ch_context[i].RX_index],
                                     AXIQ_SR_FIELD_ERROVER | AXIQ_SR_FIELD_ERRUNDER);
        if (tmp_status != 0) {
            if (tmp_status & (AXIQ_SR_FIELD_ERROVER << axiq_sr_shift(Rx_Antenna2fifo_index[rx_ch_context[i].RX_index]))) {
                g_stats.rx_stats[i][ERROR_AXIQ_FIFO_RX_OVERRUN]++;
                l1_trace(L1_TRACE_MSG_DMA_AXIQ_RX_OVER, (uint32_t)g_stats.rx_stats[i][ERROR_AXIQ_FIFO_RX_OVERRUN]);
            }
            if (tmp_status & (AXIQ_SR_FIELD_ERRUNDER << axiq_sr_shift(Rx_Antenna2fifo_index[rx_ch_context[i].RX_index]))) {
                g_stats.rx_stats[i][ERROR_AXIQ_FIFO_RX_UNDERRUN]++;
                l1_trace(L1_TRACE_MSG_DMA_AXIQ_RX_UNDER, (uint32_t)g_stats.rx_stats[i][ERROR_AXIQ_FIFO_RX_UNDERRUN]);
            }
            // l1_trace_disable = 1;
            axiq_fifo_rx_cr(AXIQ_BANK_0, Rx_Antenna2fifo_index[rx_ch_context[i].RX_index], AXIQ_CR_CLRERR, AXIQ_CR_CLRERR);
            axiq_fifo_rx_cr(AXIQ_BANK_0, Rx_Antenna2fifo_index[rx_ch_context[i].RX_index], AXIQ_CR_CLRERR, 0);
        }
    }

    // DDR DMA load test , just fetch DDR as fast as possible
    if (DDR_wr_load_start_bit_update) {
        if (!RX_ext_dma_enabled) {
            for (i = 0; i < RX_NUM_CHAN; i++) {
                ddr_wr_dma_ch_mask = dma_chan_mask(DDR_WR_DMA_CHANNEL_1 + i, 1);
                // check DDR dma completion
                if (dmac_is_complete(ddr_wr_dma_ch_mask) == ddr_wr_dma_ch_mask) {
                    dmac_clear_complete(ddr_wr_dma_ch_mask);
                    g_stats.rx_stats[i][STAT_DMA_DDR_WR]++;
                    l1_trace(L1_TRACE_MSG_DMA_DDR_WR_COMP, (uint32_t)g_stats.rx_stats[i][STAT_DMA_DDR_WR]);
                }
                // restart DDR dma if possible
                rx_ch_context[i].p_rx_ddr_enqueued = input_buffer[i];
                if (dmac_is_available(ddr_wr_dma_ch_mask) == ddr_wr_dma_ch_mask) {

                    DDR_write_multi_dma(DDR_WR_DMA_CHANNEL_1 + i, 1,
                                        rx_vspa_proxy[i].DDR_wr_base_address + rx_ch_context[i].DDR_wr_offset,
                                        2 * (uint32_t)rx_ch_context[i].p_rx_ddr_enqueued, ddr_wr_dma_xfr_size);
                    rx_ch_context[i].DDR_wr_offset =
                        (rx_ch_context[i].DDR_wr_offset + ddr_wr_dma_xfr_size) % (rx_vspa_proxy[i].DDR_wr_size);
                    l1_trace(L1_TRACE_MSG_DMA_DDR_WR_START, (uint32_t)rx_ch_context[i].p_rx_ddr_enqueued);
                }
            }
        }
    }

    // Single Tone
    if (RX_SingleT_start_bit_update) {
        for (i = 0; i < RX_NUM_CHAN; i++) {
            dma_channel_rd = Rx_Antenna2axiq_dma_chan[i + RX_index];
            axi_rd = axi_ADC_FIFO_addr[i];
            // check axiq dma completion (received new samples) and QEC them
            if (dmac_is_complete(0x1 << dma_channel_rd)) {
                dmac_clear_complete(0x1 << dma_channel_rd);
                dmac_clear_event(0x1 << dma_channel_rd);
                g_stats.rx_stats[i][STAT_DMA_AXIQ_READ]++;
                l1_trace(L1_TRACE_MSG_DMA_AXIQ_RX_COMP, (uint32_t)g_stats.rx_stats[i][STAT_DMA_AXIQ_READ]);
            }
            // restart axiq  dma if possible
            if (dmac_is_available(0x1 << dma_channel_rd)) {
                stream_read(dma_channel_rd, axi_rd, 2 * (uint32_t)(RX_SingleT_buffer + i * RX_DMA_TXR_size));
                l1_trace(L1_TRACE_MSG_DMA_AXIQ_RX_START, (uint32_t)RX_SingleT_buffer + i * RX_DMA_TXR_size);
            }
        }
    }

    // if any of previous load test is on, skip DDR streaming
    if (DDR_wr_load_start_bit_update || RX_SingleT_start_bit_update)
        goto end_rx_push;

    // Stream waveform to DDR
    if (DDR_wr_start_bit_update) {

        // check axiq dma completion
        for (i = 0; i < RX_NUM_CHAN; i++) {
            dma_channel_rd = Rx_Antenna2axiq_dma_chan[i + RX_index];
            axi_rd = axi_ADC_FIFO_addr[i];
            if (dmac_is_complete(0x1 << dma_channel_rd)) {
                dmac_clear_complete(0x1 << dma_channel_rd);
                dmac_clear_event(0x1 << dma_channel_rd);
                rx_ch_context[i].RX_total_axiq_received_size += RX_DMA_TXR_STEP;
                g_stats.rx_stats[i][STAT_DMA_AXIQ_READ]++;
                l1_trace(L1_TRACE_MSG_DMA_AXIQ_RX_COMP, (uint32_t)g_stats.rx_stats[i][STAT_DMA_AXIQ_READ]);
            }
            if (dmac_errxfr(0x1 << dma_channel_rd)) {
                dmac_clear_errxfr(0x1 << dma_channel_rd);
                g_stats.rx_stats[i][ERROR_DMA_XFER_ERROR]++;
                l1_trace(L1_TRACE_MSG_DMA_AXIQ_RX_XFER_ERROR, (uint32_t)g_stats.rx_stats[i][ERROR_DMA_XFER_ERROR]);
            }
        }

        // QEC buffer just received
        for (i = 0; i < RX_NUM_CHAN; i++) {
            if ((rx_ch_context[i].RX_total_axiq_received_size - rx_ch_context[i].RX_total_dmem_QECed_size) >= RX_DMA_TXR_STEP) {
                l1_trace(L1_TRACE_L1APP_RX_QEC_START, (uint32_t)rx_ch_context[i].p_rx_dmem_QECed);
                rx_qec_correction((vspa_complex_fixed16 *)rx_ch_context[i].p_rx_dmem_QECed,
                                  (vspa_complex_fixed16 *)rx_ch_context[i].p_rx_dmem_QECed);
                INCR_RX_BUFF(rx_ch_context[i].p_rx_dmem_QECed, i);
                rx_ch_context[i].RX_total_dmem_QECed_size += RX_DMA_TXR_STEP;
                l1_trace(L1_TRACE_L1APP_RX_QEC_COMP, (uint32_t)rx_ch_context[i].RX_total_dmem_QECed_size);
            }
        }

        // Decimation
        for (i = 0; i < RX_NUM_CHAN; i++) {
            if ((rx_ch_context[i].RX_total_dmem_QECed_size - rx_ch_context[i].RX_total_dmem_input_Decimated_size) >=
                RX_DMA_TXR_STEP) {
                // rx_busy_size = rx_ch_context[i].RX_total_dmem_output_Decimated_size -
                // rx_ch_context[i].RX_total_ddr_consumed_size;
                rx_busy_size = rx_vspa_proxy[i].la9310_fifo_produced_size - rx_vspa_proxy[i].la9310_fifo_consumed_size;
                rx_empty_size = (RX_NUM_DEC_BUF * RX_DDR_STEP) - rx_busy_size;
                if (rx_empty_size >= RX_DDR_STEP) {
                    l1_trace(L1_TRACE_L1APP_RX_DEC_START, (uint32_t)rx_ch_context[i].p_rx_dmem_QECed);
#if defined(IQMOD_RX_1T2R) || defined(IQMOD_RX_1T4R)
                    decimator_2x_8_Taps_asm((cfixed16_t *)rx_ch_context[i].p_rx_dmem_output_decimated,
                                            (cfixed16_t *)rx_ch_context[i].p_rx_dmem_input_decimated,
                                            (float32_t *)filter_taps_downsampling, (cfixed16_t *)filtState[i], RX_DMA_TXR_size);

#endif
#ifdef IQMOD_RX_1T4R
                    decimator_2x_8_Taps_asm((cfixed16_t *)rx_ch_context[i].p_rx_dmem_output_decimated,
                                            (cfixed16_t *)rx_ch_context[i].p_rx_dmem_input_decimated,
                                            (float32_t *)filter_taps_downsampling, (cfixed16_t *)filtState[i], RX_DMA_TXR_size);
#endif
                    INCR_RX_BUFF(rx_ch_context[i].p_rx_dmem_input_decimated, i);
                    INCR_RX_DEC_BUFF(rx_ch_context[i].p_rx_dmem_output_decimated, i);
                    rx_ch_context[i].RX_total_dmem_input_Decimated_size += RX_DMA_TXR_STEP;
                    // rx_ch_context[i].RX_total_dmem_output_Decimated_size+= RX_DDR_STEP;
                    rx_vspa_proxy[i].la9310_fifo_produced_size += RX_DDR_STEP;
                    rx_proxy_updated = 1;
                    l1_trace(L1_TRACE_L1APP_RX_DEC_COMP, (uint32_t)rx_ch_context[i].RX_total_dmem_input_Decimated_size);
                }
            }
        }

        // Push data to DDR if standalone mode
        if (!RX_ext_dma_enabled) {
            for (i = 0; i < RX_NUM_CHAN; i++) {
                // check DDR dma completion
                ddr_wr_dma_ch_mask = dma_chan_mask(DDR_WR_DMA_CHANNEL_1 + i, 1);
                if (dmac_is_complete(ddr_wr_dma_ch_mask) == ddr_wr_dma_ch_mask) {
                    dmac_clear_complete(ddr_wr_dma_ch_mask);
                    rx_vspa_proxy[i].la9310_fifo_consumed_size += RX_DDR_STEP;
                    g_stats.rx_stats[i][STAT_DMA_DDR_WR]++;
                    l1_trace(L1_TRACE_MSG_DMA_DDR_WR_COMP, (uint32_t)g_stats.rx_stats[i][STAT_DMA_DDR_WR]);
                }

                // host flow control
                if ((rx_ch_context[i].RX_total_ddr_enqueued_size - tx_vspa_proxy.host_consumed_size[i] <
                     rx_vspa_proxy[i].DDR_wr_size) ||
                    host_flow_control_disable) {
                    // if ((rx_ch_context[i].RX_total_dmem_output_Decimated_size-rx_ch_context[i].RX_total_ddr_enqueued_size) >=
                    // RX_DDR_STEP)
                    if ((rx_vspa_proxy[i].la9310_fifo_produced_size - rx_ch_context[i].RX_total_ddr_enqueued_size) >= RX_DDR_STEP) {
                        if (dmac_is_available(ddr_wr_dma_ch_mask) == ddr_wr_dma_ch_mask) {
                            DDR_write_multi_dma(DDR_WR_DMA_CHANNEL_1 + i, 1,
                                                rx_vspa_proxy[i].DDR_wr_base_address + rx_ch_context[i].DDR_wr_offset,
                                                2 * (uint32_t)rx_ch_context[i].p_rx_ddr_enqueued, ddr_wr_dma_xfr_size);
                            INCR_RX_DEC_BUFF(rx_ch_context[i].p_rx_ddr_enqueued, i);
                            rx_ch_context[i].RX_total_ddr_enqueued_size += RX_DDR_STEP;
                            rx_ch_context[i].DDR_wr_offset += ddr_wr_dma_xfr_size;
                            if (rx_ch_context[i].DDR_wr_offset >= rx_vspa_proxy[i].DDR_wr_size)
                                rx_ch_context[i].DDR_wr_offset = 0;
                            l1_trace(L1_TRACE_MSG_DMA_DDR_WR_START, (uint32_t)rx_ch_context[i].p_rx_ddr_enqueued);
                        }
                    }
                }
            }
        }

        // restart axiq  dma if possible
        for (i = 0; i < RX_NUM_CHAN; i++) {
            dma_channel_rd = Rx_Antenna2axiq_dma_chan[i + RX_index];
            if (dmac_is_available(0x1 << dma_channel_rd)) {
                axi_rd = axi_ADC_FIFO_addr[i];
                rx_busy_size = rx_ch_context[i].RX_total_axiq_enqueued_size - rx_ch_context[i].RX_total_dmem_input_Decimated_size;
                rx_empty_size = (RX_NUM_BUF * RX_DMA_TXR_STEP) - rx_busy_size;
                if (rx_empty_size >= RX_DMA_TXR_STEP) {
                    stream_read(dma_channel_rd, axi_rd, 2 * (uint32_t)(rx_ch_context[i].p_rx_axiq_enqueued));
                    INCR_RX_BUFF(rx_ch_context[i].p_rx_axiq_enqueued, i);
                    rx_ch_context[i].RX_total_axiq_enqueued_size += RX_DMA_TXR_STEP;
                    l1_trace(L1_TRACE_MSG_DMA_AXIQ_RX_START, (uint32_t)rx_ch_context[i].p_rx_axiq_enqueued);
                } else {
                    // overflow no more dmem buffer to arm new axiq DMA
                    g_stats.rx_stats[i][ERROR_DMA_DDR_WR_OVERRUN]++;
                    l1_trace(L1_TRACE_MSG_DMA_DDR_WR_OVERRUN, (uint32_t)g_stats.rx_stats[i][ERROR_DMA_DDR_WR_OVERRUN]);
                    // l1_trace_disable = 1;
                }
            }
        }

        // end reception
        all_chan_ddr_consumed_size = 0;
        for (i = 0; i < RX_NUM_CHAN; i++) {
            if (rx_vspa_proxy[i].la9310_fifo_consumed_size >= rx_vspa_proxy[i].DDR_wr_size)
                all_chan_ddr_consumed_size++;
        }
        if ((all_chan_ddr_consumed_size >= RX_NUM_CHAN) && (!DDR_wr_continuous)) {
            DDR_wr_start_bit_update = 0;
            for (i = 0; i < RX_NUM_CHAN; i++) {
                rx_vspa_proxy[i].DDR_wr_base_address = 0xdeadbeef;
                dma_channel_rd = Rx_Antenna2axiq_dma_chan[i + RX_index];
                axi_rd = axi_ADC_FIFO_addr[i];
                dmac_abort(0x1 << dma_channel_rd);
                dmac_clear_complete(0x1 << dma_channel_rd);
                axiq_fifo_rx_disable(AXIQ_BANK_0, Rx_Antenna2fifo_index[rx_ch_context[i].RX_index]);
            }
            rx_proxy_updated = 1;
            mailbox_out_msg_0_MSB = 0;
            mailbox_out_msg_0_LSB = 0x1;
            host_mbox0_post(MAKEDWORD(mailbox_out_msg_0_MSB, mailbox_out_msg_0_LSB));
        }
    }

end_rx_push:
    /* Check DMA errors*/
    tmp_dma_errors = (uint32_t)iord(DMA_CFGERR_STAT);
    if (tmp_dma_errors != 0) {
        l1_trace(L1_TRACE_MSG_DMA_CFGERR, (uint32_t)(g_stats.rx_stats[i][ERROR_DMA_CONFIG_ERROR]));
        g_stats.rx_stats[i][ERROR_DMA_CONFIG_ERROR]++;
        dmac_clear_errcfg();
        while (dbg_gbl == 6) {
        };
    };

    // update host proxy if needed
    VSPA_PROXY_update();
}
#pragma optimize_for_size reset
