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
#include "ippu.h"
#include "dmac.h"
#include "iohw.h"
#include "main.h"
#include "dfe.h"
#include "diffft.h"
#include "l1-trace.h"
#include "iqmod_tx.h"
#include "iqmod_rx.h"
#include "stats.h"

float y[FFT_SIZE] _VSPA_VECTOR_ALIGN;
static float log_norm = 10;

#ifndef IQMOD_RX_1T0R
static vspa_complex_fixed16 *intput_buffer_1 = (vspa_complex_fixed16 *)input_buffer + RX_DMA_TXR_size;
static vspa_complex_fixed16 *input_buffer_2 = (vspa_complex_fixed16 *)input_buffer + 2 * RX_DMA_TXR_size;
#endif

vspa_complex_fixed16 *TX_SingleT_buffer;
vspa_complex_fixed16 *RX_SingleT_buffer;

#ifndef IQMOD_RX_0T1R
__attribute__(( section(".text.opcode_1") ))
#pragma optimization_level 0
void gen_nco_single_tone(vspa_complex_fixed16 *buffer) __attribute__ ((aligned(64)))
{
    uint32_t i;

    gain.real = 0.9;
    gain.imag = 0;
    __clr_VRA();
    __set_prec(single, single, single, single, half_fixed);
    __set_Smode(S0word, S1nco, S2zeros);
    __set_VRAptr_rV(_VR2);
    __set_VRAptr_rSt(2);
    __set_VRAincr_rV(_VRH);
    __set_range1_rV(2 * _VR, 2 * _VR + _VRH);
    __set_nco(normal, 0x1, 0);
    pgain = &gain;
    __ld_Rx_mem_unaligned(0, pgain);
    tone_freq_DAC = ((fft_bin << 23) ^ 0xFFFFFFFF) + 1;
    __set_nco_freq(tone_freq_DAC);
    for (i = 0; i < MEM_LINE_SIZE; i++) {
        tone_phase_DAC = i * 32;
        __set_nco_phase(tone_phase_DAC);
        __rd_S0();
        __rd_S1();
        __rd_S2();
        __cmad();
        __wr(hlinecplx);
        tone_phase_DAC += 16;
        __set_nco_phase(tone_phase_DAC);
        __rd_S0();
        __rd_S1();
        __rd_S2();
        __cmad();
        __wr(hlinecplx);
        __st_vec((vspa_vector_pair_fixed16 *)buffer + i);
    }
    // generated data is 2's complement, force DMA to do conversions

#ifdef TXIQCOMP2
    // using only one batch, need to change default circular buffer config
    iq_comp_params2_tx.inpCircBuffBase = (vspa_complex_fixed16 *)buffer;
    iq_comp_params2_tx.inpCircBuffSize = FFT_SIZE * 2;
#endif
    tx_qec_correction(buffer, buffer);

    return;
}
#pragma optimization_level reset

__attribute__((section(".text.opcode_1"))) void TX_single_tone(void) __attribute__((aligned(64))) {
    uint64_t msg64 = host_mbox0_read();
    uint32_t cmd_start = (HIWORD(msg64)) & 0x00100000;

    if (cmd_start) {
        if (TX_SingleT_start_bit_update || (DDR_rd_start_bit_update && !DDR_rd_load_start_bit_update))
            goto fail_tx_single_tone;

        TX_SingleT_start_bit_update = 1;
#if TX_QEC_INPLACE
        TX_SingleT_buffer = output_buffer;
#else
        TX_SingleT_buffer = output_qec_buffer;
#endif
        gen_nco_single_tone(TX_SingleT_buffer);

        dmac_clear_complete(0x1 << DMA_CHANNEL_WR);
        dmac_clear_complete(0x1 << DMA_CHANNEL_WR);
        mailbox_out_msg_0_MSB = 0;
        mailbox_out_msg_0_LSB = 0x1;
        host_mbox0_post(MAKEDWORD(mailbox_out_msg_0_MSB, mailbox_out_msg_0_LSB));

        axiq_tx_disable();
        stream_write_ptr_rst(DMA_CHANNEL_WR, axi_wr);
        do {
#ifndef IQMOD_RX_1T0R
            PUSH_RX_DATA();
#endif
        } while (!dmac_is_complete(0x1 << DMA_CHANNEL_WR));
        dmac_clear_complete(0x1 << DMA_CHANNEL_WR);
        stream_write(DMA_CHANNEL_WR, axi_wr, 2 * (uint32_t)(TX_SingleT_buffer));
        stream_write(DMA_CHANNEL_WR, axi_wr, 2 * (uint32_t)(TX_SingleT_buffer));
        g_stats.tx_stats[STAT_DMA_AXIQ_WRITE] += 2;

        axiq_fifo_tx_enable(AXIQ_BANK_0, AXIQ_FIFO_TX0);

        return;
    }

    if (!cmd_start) /* stop */
    {
        if (!TX_SingleT_start_bit_update)
            goto fail_tx_single_tone;
        axiq_tx_disable();
        TX_SingleT_start_bit_update = 0;
        dmac_abort(0x1 << DMA_CHANNEL_WR);
        dmac_clear_complete(0x1 << DMA_CHANNEL_WR);
        dmac_clear_event(0x1 << DMA_CHANNEL_WR);
        mailbox_out_msg_0_MSB = 0;
        mailbox_out_msg_0_LSB = 0x1;
        host_mbox0_post(MAKEDWORD(mailbox_out_msg_0_MSB, mailbox_out_msg_0_LSB));
        return;
    }

fail_tx_single_tone:

    /* failure send Nack */
    mailbox_out_msg_0_MSB = 0;
    mailbox_out_msg_0_LSB = 0x0;
    host_mbox0_post(MAKEDWORD(mailbox_out_msg_0_MSB, mailbox_out_msg_0_LSB));
    return;
}
#endif

#ifndef IQMOD_RX_1T1R
void RX_single_tone_measurement(void) { return; }
// void TX_single_tone(void){return;}
// void gen_nco_single_tone(vspa_complex_fixed16 *buffer){return;}
#else
__attribute__((section(".text.opcode_2"))) void RX_single_tone_measurement(void) {
    uint64_t msg64 = host_mbox0_read();
    uint32_t cmd_start = (HIWORD(msg64)) & 0x00100000;
    uint32_t i;

    if (cmd_start) {
        if (RX_SingleT_start_bit_update || (DDR_wr_start_bit_update && !DDR_wr_load_start_bit_update))
            goto fail_rx_single_tone;

        RX_SingleT_continue = (HIWORD(msg64)) & 0x00200000;
        // AXIQ read simple load test
        if (RX_SingleT_continue) {
            // DDR_wr_CMP_enable= (HIWORD(msg64)) & 0x00080000;
            ddr_wr_dma_ch_nb = ((HIWORD(msg64)) & 0x00070000) >> 16;
            if (ddr_wr_dma_ch_nb > 2)
                goto fail_rx_single_tone;
            if (!ddr_wr_dma_ch_nb) {
                // default 1 DMA write 526MB/s half duplex, 490MB/s full duplex
                ddr_wr_dma_ch_nb = 1;
            }
            ddr_wr_dma_ch_mask = dma_chan_mask(DDR_WR_DMA_CHANNEL_1, ddr_wr_dma_ch_nb);

            RX_SingleT_start_bit_update = 1;

            RX_SingleT_buffer = (vspa_complex_fixed16 *)input_buffer;

            // stream first samples
            for (i = 0; i < RX_NUM_CHAN; i++) {
                dma_channel_rd = Rx_Antenna2axiq_dma_chan[i + RX_index];
                axi_rd = axi_ADC_FIFO_addr[i];
                stream_read(dma_channel_rd, axi_rd, 2 * (uint32_t)(RX_SingleT_buffer + i * RX_DMA_TXR_size));
                l1_trace(L1_TRACE_MSG_DMA_AXIQ_RX_START, (uint32_t)RX_SingleT_buffer + i * RX_DMA_TXR_size);
                stream_read(dma_channel_rd, axi_rd, 2 * (uint32_t)(RX_SingleT_buffer + i * RX_DMA_TXR_size));
                l1_trace(L1_TRACE_MSG_DMA_AXIQ_RX_START, (uint32_t)RX_SingleT_buffer + i * RX_DMA_TXR_size);
            }

            // ack stream start
            mailbox_out_msg_0_MSB = 0;
            mailbox_out_msg_0_LSB = 0x1;
            host_mbox0_post(MAKEDWORD(mailbox_out_msg_0_MSB, mailbox_out_msg_0_LSB));

            // enable fifo
            for (i = 0; i < RX_NUM_CHAN; i++) {
                dma_channel_rd = Rx_Antenna2axiq_dma_chan[i + RX_index];
                axiq_fifo_rx_enable(AXIQ_BANK_0, Rx_Antenna2fifo_index[i]);
                axiq_fifo_rx_cr(AXIQ_BANK_0, Rx_Antenna2fifo_index[i], AXIQ_CR_CLRERR, AXIQ_CR_CLRERR);
                axiq_fifo_rx_cr(AXIQ_BANK_0, Rx_Antenna2fifo_index[i], AXIQ_CR_CLRERR, 0);
                dmac_clear_errxfr(0x1 << dma_channel_rd);
            }

            return;
        }

#if 0
		// single measurment
		axiq_fifo_rx_disable(AXIQ_BANK_0, Rx_Antenna2fifo_index[RX_index]);

		stream_read_ptr_rst(dma_channel_rd, axi_rd);
		do {
			PUSH_TX_DATA();
		} while (!dmac_is_complete(0x1<<dma_channel_rd));
		dmac_clear_complete(0x1<<dma_channel_rd);

		stream_read(dma_channel_rd, axi_rd, 2 * (uint32_t)(input_buffer));
		axiq_fifo_rx_enable(AXIQ_BANK_0, Rx_Antenna2fifo_index[RX_index]);
		do {
			PUSH_TX_DATA();
		} while (!dmac_is_complete(0x1<<dma_channel_rd));
		dmac_clear_complete(0x1<<dma_channel_rd);

		fftDIF512_hfx_hfx(input_buffer, (vspa_complex_fixed16 *)input_buffer_1, input_buffer, FFT_SIZE*2);
		bitRev512Invoke(
					(vspa_complex_float16 *)input_buffer_2,
					(vspa_complex_float16 *)input_buffer_1
				);
		do { /* wait */ } while (!ippu_is_done());

		PUSH_TX_DATA();

		// calculate the power of FFT samples
		__set_prec(half_fixed, half_fixed, half_fixed, single, single);
		__set_Smode(S0straight, S1straight, S2zeros);
		__clr_VRA();
		__set_VRAptr_rS2(_VR1);
		__set_VRAptr_rV(_VR2);
		__set_VRAptr_rSt(2);
		for (i = 0; i < MEM_LINE_SIZE; i++) {
			__ld_Rx_mem(0, (const vspa_vector_pair_fixed16 *)input_buffer_2+i);
			__rd_S0();
			__rd_S1();
			__rd_S2();
			__cmad();
			__wr(even);
			__st_vec((vspa_vector_pair_fixed16 *)y+i);
		}
		int S_N_ratio_th_dB;

		S_N_ratio_th_dB = ((mailbox_in_msg_0_LSB & 0x0000FF00) >> 8);
		//calculate signal and noise powers
		float S = y[tone_index];
		float *pS;

		__set_prec_single();
		__set_Smode(S0hword, S1real1, S2zeros);
		vspa_complex_float32 *py;
		float *pN;
		float   N;

		pN =  &N;
		py = (vspa_complex_float32 *)(y);
		__ld_Rx_mem_unaligned(0, py);
		__rd_S0();
		__rd_S1();
		__rmad();
		__wr(hlinecplx);

		PUSH_TX_DATA();

		for (i = 1; i < FFT_SIZE; i++) {
			if (i != tone_index && i != image_tone_index && i != FFT_SIZE/2)
				{
				py = (vspa_complex_float32 *)(y+i);
				__ld_Rx_mem_unaligned(0, py);
				__rd_S0();
				__rd_S1();
				__rd_S2();
				__rmac();
				__wr(hlinecplx);
			}

			if ((i % 16) == 0)
			{
				PUSH_TX_DATA();
			}
		}

		PUSH_TX_DATA();

		__st_w_asm(pN);
		//ratio
		float *pS_N_ratio;
		float   S_N_ratio;

		pS_N_ratio =  &S_N_ratio;
		pN =  &N;
		pS =  &S;
		__set_Smode(S0hword, S1straight, S2zeros);
		__set_VRAptr_rS0(_VR0);
		__set_VRAptr_rS1(_VR1);
		__set_VRAptr_rS2(_VR1);
		__ld_Rx_mem_unaligned(0, pS);
		__ld_Rx_mem_unaligned(1, pN);
		__rd_S0();
		__rd_S1();
		__rcp();
		__rmad_sau();
		__wr(hlinecplx);
		__st_w_asm(pS_N_ratio);
		int log_result1, log_result2, S_N_ratio_dB;

		__log2(log_result1, S_N_ratio);
		__log2(log_result2, log_norm);
		S_N_ratio_dB = (10*log_result1) / (log_result2);
		if (S_N_ratio_dB <= 0)
		{
			S_N_ratio_dB = 0;
		}
		dmac_clear_event(0x1<<dma_channel_rd);
		mailbox_out_msg_0_MSB = (S_N_ratio_dB & 0xFF);
		mailbox_out_msg_0_LSB = 0x1;
		host_mbox0_post(MAKEDWORD(mailbox_out_msg_0_MSB, mailbox_out_msg_0_LSB));
		return;
#endif
    }

    if (!cmd_start) {
        if (!RX_SingleT_start_bit_update)
            goto fail_rx_single_tone;
        RX_SingleT_start_bit_update = 0;
        dmac_abort(0x1 << dma_channel_rd);
        dmac_clear_complete(0x1 << dma_channel_rd);
        axiq_fifo_rx_disable(AXIQ_BANK_0, Rx_Antenna2fifo_index[RX_index]);
        mailbox_out_msg_0_MSB = 0;
        mailbox_out_msg_0_LSB = 0x1;
        host_mbox0_post(MAKEDWORD(mailbox_out_msg_0_MSB, mailbox_out_msg_0_LSB));
        return;
    }

fail_rx_single_tone:
    /* failure send Nack */
    mailbox_out_msg_0_MSB = 0;
    mailbox_out_msg_0_LSB = 0x0;
    host_mbox0_post(MAKEDWORD(mailbox_out_msg_0_MSB, mailbox_out_msg_0_LSB));
    return;
}

#endif
