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
#include "iqmod_rx.h"
#include "iqmod_tx.h"
#include "cal_signal.h"
#include "axiq.h"
#include "dfe.h"
#include "diffft.h"

#ifndef IQMOD_RX_1T1R

void BW_CAL(void) { return; }
void RX_DCOC_CAL(void) { return; }
void TX_DCOC_CAL(void) { return; }

#else

uint32_t tone_index_vec_1[3] = { 8, 16, 32 };
uint32_t tone_index_vec_2[3] = { 32, 64, 128 };
float TX_DCOC_power_new, TX_DCOC_power_old, power_sum;
static float log_norm = 10;

static vspa_complex_fixed16 *output_buffer_0 = &output_buffer[0 * FFT_SIZE];
static vspa_complex_fixed16 *output_buffer_1 = &output_buffer[1 * FFT_SIZE];
vspa_complex_fixed16 *input_buffer_0 = &input_buffer[0 * FFT_SIZE];
vspa_complex_fixed16 *input_buffer_1 = &input_buffer[1 * FFT_SIZE];

__attribute__((section(".text.opcode_3"))) void RX_DCOC_CAL(void) __attribute__((aligned(64))) {
#if 0
	axiq_fifo_rx_disable(AXIQ_BANK_0, Rx_Antenna2fifo_index[RX_index]);
	stream_read_ptr_rst(dma_channel_rd, axi_rd);
	WAIT(dmac_is_complete(0x1<<dma_channel_rd));
	dmac_clear_complete(0x1<<dma_channel_rd);
	stream_read(dma_channel_rd, axi_rd, 2 * (uint32_t)(input_buffer));
	stream_read(dma_channel_rd, axi_rd, 2 * (uint32_t)(input_buffer));
	axiq_fifo_rx_enable(AXIQ_BANK_0, Rx_Antenna2fifo_index[RX_index]);
	WAIT(dmac_is_complete(0x1<<dma_channel_rd));
	dmac_clear_complete(0x1<<dma_channel_rd);
	WAIT(dmac_is_complete(0x1<<dma_channel_rd));
	dmac_clear_complete(0x1<<dma_channel_rd);

	fftDIF512_hfx_hfx(input_buffer, (vspa_complex_fixed16 *)output_buffer_0, input_buffer, FFT_SIZE*2);
	bitRev512Invoke(
				(vspa_complex_float16 *)output_buffer_1,
				(vspa_complex_float16 *)output_buffer_0
			);
	do { /* wait */ } while (!ippu_is_done());

	dmac_clear_event(0x1<<dma_channel_rd);
#ifdef RXIQCOMP2
	iq_comp_params2_rx.dcOffset.real = output_buffer_1[FFT_SIZE/2].real;
	iq_comp_params2_rx.dcOffset.imag = output_buffer_1[FFT_SIZE/2].imag;
	mailbox_out_msg_0_MSB =  *(uint32_t *) &iq_comp_params2_rx.dcOffset;
#else
	rxiqcompcfg_struct.dcOffset.real = output_buffer_1[FFT_SIZE/2].real;
	rxiqcompcfg_struct.dcOffset.imag = output_buffer_1[FFT_SIZE/2].imag;
	mailbox_out_msg_0_MSB =  *(uint32_t *) &rxiqcompcfg_struct.dcOffset;
#endif
	mailbox_out_msg_0_LSB = 0x1;

	host_mbox0_post(MAKEDWORD(mailbox_out_msg_0_MSB, mailbox_out_msg_0_LSB));

#endif
}

__attribute__((section(".text.opcode_3"))) void TX_DCOC_CAL(void) {
#if 0

	int f_diff_index = FFT_SIZE/2 - 32, i;
	int resume_index = 1;
	float step_size = 0.0009765625;// 0.00048828125;//0.000977;

	TX_DCOC_power_old = 0;
	TX_DCOC_power_new = 0;
#ifdef TXIQCOMP2
				iq_comp_params2_tx.dcOffset.real = 0;
				iq_comp_params2_tx.dcOffset.imag = 0;
#else
				txiqcompcfg_struct.dcOffset.real = 0;
				txiqcompcfg_struct.dcOffset.imag = 0;
#endif
	gain.real = 0;
	gain.imag = 0;
	dmac_abort(0x1<<DMA_CHANNEL_WR);
	dmac_clear_complete(0x1<<DMA_CHANNEL_WR);
	__set_nco(normal, 0x1, 0);
	tone_freq_DAC = ((0 << 23) ^ 0xFFFFFFFF) + 1;
	__set_nco_freq(tone_freq_DAC);
	for (int indexx = 0 ; indexx < 5 ; indexx++)
	{
		if (indexx == 1)
		{
			resume_index = 1;
			gain.real += step_size;
		} else if (indexx == 2)
		{
			resume_index = 1;
			gain.real -= step_size;
		} else if (indexx == 3)
		{
			resume_index = 1;
			gain.imag += step_size;
		} else if (indexx == 4)
		{
			resume_index = 1;
			gain.imag -= step_size;
		}
		while (resume_index == 1)
		{
			__clr_VRA();
			pgain =  &gain;
			__set_prec(single, single, single, single, half_fixed);
			__set_Smode(S0word, S1nco, S2zeros);
			__set_VRAptr_rV(_VR2);
			__set_VRAptr_rSt(2);
			__set_VRAincr_rV(_VRH);
			__set_range1_rV(2*_VR, 2*_VR+_VRH);
			__ld_Rx_mem_unaligned(0, pgain);
			__rd_S0();
			__rd_S2();
			for (i = 0; i < MEM_LINE_SIZE; i++)
			{
				tone_phase_DAC = i*32;
				__set_nco_phase(tone_phase_DAC);
				__rd_S1();
				__cmad();
				__wr(hlinecplx);
				tone_phase_DAC += 16;
				__set_nco_phase(tone_phase_DAC);
				__rd_S1();
				__cmad();
				__wr(hlinecplx);
				__st_vec((vspa_vector_pair_fixed16 *)output_buffer+i);
			}
			power_sum = 0;
			for (int counter = 0 ; counter < FFT_SIZE*10 ; counter++)
			{
				axiq_tx_disable();
				axiq_fifo_rx_disable(AXIQ_BANK_0, Rx_Antenna2fifo_index[RX_index]);
				stream_write_ptr_rst(DMA_CHANNEL_WR, axi_wr);
				WAIT(dmac_is_complete(0x1<<DMA_CHANNEL_WR));
				dmac_clear_complete(0x1<<DMA_CHANNEL_WR);
				stream_read_ptr_rst(dma_channel_rd, axi_rd);
				WAIT(dmac_is_complete(0x1<<dma_channel_rd));
				dmac_clear_complete(0x1<<dma_channel_rd);
				stream_write(DMA_CHANNEL_WR, axi_wr, 2 * (uint32_t)(output_buffer));
				stream_write(DMA_CHANNEL_WR, axi_wr, 2 * (uint32_t)(output_buffer));
				// DMA transfer from AXIQ to DMEM
				stream_read(dma_channel_rd, axi_rd, 2 * (uint32_t)(input_buffer));
				stream_read(dma_channel_rd, axi_rd, 2 * (uint32_t)(input_buffer));
				axiq_fifo_tx_enable(AXIQ_BANK_0, AXIQ_FIFO_TX0);
				axiq_fifo_rx_enable(AXIQ_BANK_0, Rx_Antenna2fifo_index[RX_index]);
				WAIT(dmac_is_complete(0x1<<dma_channel_rd));
				dmac_clear_complete(0x1<<dma_channel_rd);
				WAIT(dmac_is_complete(0x1<<dma_channel_rd));
				dmac_clear_complete(0x1<<dma_channel_rd);

				fftDIF512_hfx_hfx(input_buffer, (vspa_complex_fixed16 *)output_buffer_0, input_buffer, FFT_SIZE*2);

				bitRev512Invoke(
							(vspa_complex_float16 *)output_buffer_1,
							(vspa_complex_float16 *)output_buffer_0
						);
				do { /* wait */ } while (!ippu_is_done());

				// calculate the power of FFT samples
				__set_prec(half_fixed, half_fixed, half_fixed, single, single);
				__set_Smode(S0straight, S1straight, S2zeros);
				__clr_VRA();
				__set_VRAptr_rS2(_VR1);
				__set_VRAptr_rV(_VR2);
				__set_VRAptr_rSt(2);
				for (i = 0; i < MEM_LINE_SIZE; i++)
				{
					__ld_Rx_mem(0, (const vspa_vector_pair_fixed16 *)output_buffer_1+i);
					__rd_S0();
					__rd_S1();
					__rd_S2();
					__cmad();
					__wr(even);
					__st_vec((vspa_vector_pair_fixed16 *)y+i);
				}
				power_sum += y[f_diff_index];
			}
			if (indexx == 0)
			{
				TX_DCOC_power_old = power_sum;
				resume_index = 0;
			} else if (indexx == 1)
			{
				TX_DCOC_power_new = power_sum;
				if ((*(uint32_t *)&TX_DCOC_power_new) < (*(uint32_t *)&TX_DCOC_power_old))
				{
					TX_DCOC_power_old = TX_DCOC_power_new;
					gain.real += step_size;
				} else
				{
					resume_index = 0;
					gain.real -= step_size;
				}
			} else if (indexx == 2)
			{
				TX_DCOC_power_new = power_sum;
				if ((*(uint32_t *)&TX_DCOC_power_new) < (*(uint32_t *)&TX_DCOC_power_old))
				{
					TX_DCOC_power_old = TX_DCOC_power_new;
					gain.real -= step_size;
				} else
				{
					resume_index = 0;
					gain.real += step_size;
				}
			} else if (indexx == 3)
			{
				TX_DCOC_power_new = power_sum;
				if ((*(uint32_t *)&TX_DCOC_power_new) < (*(uint32_t *)&TX_DCOC_power_old))
				{
					TX_DCOC_power_old = TX_DCOC_power_new;
					gain.imag += step_size;
				} else
				{
					resume_index = 0;
					gain.imag -= step_size;
				}
			} else if (indexx == 4)
			{
				TX_DCOC_power_new = power_sum;
				if ((*(uint32_t *)&TX_DCOC_power_new) < (*(uint32_t *)&TX_DCOC_power_old))
				{
					TX_DCOC_power_old = TX_DCOC_power_new;
					gain.imag -= step_size;
				} else
				{
					resume_index = 0;
					gain.imag += step_size;
				}
			}
		}
	}
	pgain =  &gain;
	__set_prec(single, single, single, single, half_fixed);
	__set_Smode(S0straight, S1real1, S2zeros);
	__clr_VRA();
	__set_VRAptr_rV(_VR2);
	__set_VRAptr_rSt(2);
	__ld_Rx_mem_unaligned(0, pgain);
	__rd_S0();
	__rd_S1();
	__rd_S2();
	__rmad();
	__wr(straight);
#ifdef TXIQCOMP2
	__st_w_asm(&iq_comp_params2_tx.dcOffset);
#else
	__st_w_asm(&txiqcompcfg_struct.dcOffset);
#endif
	dmac_clear_event(0x1<<dma_channel_rd);
	dmac_clear_event(0x1<<DMA_CHANNEL_WR);
#ifdef TXIQCOMP2
	mailbox_out_msg_0_MSB =  *(uint32_t *) &iq_comp_params2_tx.dcOffset;
#else
	mailbox_out_msg_0_MSB =  *(uint32_t *) &txiqcompcfg_struct.dcOffset;
#endif
	mailbox_out_msg_0_LSB = 0x1;
	host_mbox0_post(MAKEDWORD(mailbox_out_msg_0_MSB, mailbox_out_msg_0_LSB));
#endif
}

__attribute__((section(".text.opcode_4"))) void BW_CAL(void) {
#if 0
	uint32_t i;

	gain.real = 0.35;
	gain.imag = 0;
	// 1st tone
	__clr_VRA();
	__set_prec(single, single, single, single, half_fixed);
	__set_Smode(S0word, S1nco, S2zeros);
	__set_VRAptr_rV(_VR2);
	__set_VRAptr_rSt(2);
	__set_VRAincr_rV(_VRH);
	__set_range1_rV(2*_VR, 2*_VR+_VRH);
	__set_nco(normal, 0x1, 0);
	pgain =  &gain;
	__ld_Rx_mem_unaligned(0, pgain);
	uint32_t  nco_freq_1 = ((tone_index_vec_1[BW_index] << 23) ^ 0xFFFFFFFF) + 1;

	__set_nco_freq(nco_freq_1);
	for (i = 0; i < FFT_SIZE/32; i++)
	{
		tone_phase_DAC = i*32;
		__set_nco_phase(tone_phase_DAC);
		__rd_S0();
		__rd_S1();
		__cmad();
		__wr(hlinecplx);
		tone_phase_DAC += 16;
		__set_nco_phase(tone_phase_DAC);
		__rd_S0();
		__rd_S1();
		__cmad();
		__wr(hlinecplx);
		__st_vec((vspa_vector_pair_fixed16 *)output_buffer_0+i);
	}
	// 2nd tone
	pgain =  &gain;
	__set_nco(normal, 0x1, 0);
	__ld_Rx_mem_unaligned(0, pgain);
	uint32_t  nco_freq_2 = ((tone_index_vec_2[BW_index] << 23) ^ 0xFFFFFFFF) + 1;

	__set_nco_freq(nco_freq_2);
	for (i = 0; i < FFT_SIZE/32; i++)
	{
		tone_phase_DAC = i*32;
		__set_nco_phase(tone_phase_DAC);
		__rd_S0();
		__rd_S1();
		__cmad();
		__wr(hlinecplx);
		tone_phase_DAC += 16;
		__set_nco_phase(tone_phase_DAC);
		__rd_S0();
		__rd_S1();
		__cmad();
		__wr(hlinecplx);
		__st_vec((vspa_vector_pair_fixed16 *)output_buffer_1+i);
	}
	// add
	__clr_VRA();
	__set_prec(half_fixed, half_fixed, half_fixed, single, half_fixed);
	__set_Smode(S0hlinecplx, S1cplx1, S2hlinecplx);
	__set_VRAincr_rS0(_VRH);
	__set_range1_rS0(0, _VRH);
	__set_VRAptr_rS2(_VR1);
	__set_VRAincr_rS2(_VRH);
	__set_range1_rS2(_VR, _VR+_VRH);
	__set_VRAptr_rV(_VR2);
	__set_VRAptr_rSt(2);
	__set_VRAincr_rV(_VRH);
	__set_range1_rV(2*_VR, 2*_VR+_VRH);
	for (i = 0; i < MEM_LINE_SIZE; i++)
	{
		__ld_Rx_mem(0, (const vspa_vector_pair_fixed16 *)output_buffer_0+i);
		__ld_Rx_mem(1, (const vspa_vector_pair_fixed16 *)output_buffer_1+i);
		__rd_S0();
		__rd_S1();
		__rd_S2();
		__cmad();
		__wr(hlinecplx);
		__rd_S0();
		__rd_S1();
		__rd_S2();
		__cmad();
		__wr(hlinecplx);
		__st_vec((vspa_vector_pair_fixed16 *)output_buffer_0+i);
	}
	axiq_tx_disable();
	axiq_fifo_rx_disable(AXIQ_BANK_0, Rx_Antenna2fifo_index[RX_index]);
	stream_write_ptr_rst(DMA_CHANNEL_WR, axi_wr);
	stream_read_ptr_rst(dma_channel_rd, axi_rd);
	WAIT(dmac_is_complete(0x1<<DMA_CHANNEL_WR));
	dmac_clear_complete(0x1<<DMA_CHANNEL_WR);
	WAIT(dmac_is_complete(0x1<<dma_channel_rd));
	dmac_clear_complete(0x1<<dma_channel_rd);
	stream_write(DMA_CHANNEL_WR, axi_wr, 2 * (uint32_t)(output_buffer_0));
	stream_write(DMA_CHANNEL_WR, axi_wr, 2 * (uint32_t)(output_buffer_0));
	// DMA transfer from AXIQ to DMEM
	stream_read(dma_channel_rd, axi_rd, 2 * (uint32_t)(input_buffer_0));
	axiq_fifo_tx_enable(AXIQ_BANK_0, AXIQ_FIFO_TX0);
	axiq_fifo_rx_enable(AXIQ_BANK_0, Rx_Antenna2fifo_index[RX_index]);
	WAIT(dmac_is_complete(0x1<<dma_channel_rd));
	dmac_clear_complete(0x1<<dma_channel_rd);
	fftDIF512_hfx_hfx(input_buffer_0, (vspa_complex_fixed16 *)input_buffer_1, input_buffer_0, FFT_SIZE*2);

	bitRev512Invoke(
				(vspa_complex_float16 *)input_buffer_0,
				(vspa_complex_float16 *)input_buffer_1
			);
	do { /* wait */ } while (!ippu_is_done());

	// calculate the power of FFT samples
	__set_prec(half_fixed, half_fixed, half_fixed, single, single);
	__set_Smode(S0straight, S1straight, S2zeros);
	__clr_VRA();
	__set_VRAptr_rS2(_VR1);
	__set_VRAptr_rV(_VR2);
	__set_VRAptr_rSt(2);
	for (i = 0; i < MEM_LINE_SIZE; i++)
	{
		__ld_Rx_mem(0, (const vspa_vector_pair_fixed16 *)input_buffer_0+i);
		__rd_S0();
		__rd_S1();
		__rd_S2();
		__cmad();
		__wr(even);
		__st_vec((vspa_vector_pair_fixed16 *)y+i);
	}
	//calculate first and second tone powers
	int tone_index_1 = FFT_SIZE/2 + tone_index_vec_1[BW_index];
	int tone_index_2 = FFT_SIZE/2 + tone_index_vec_2[BW_index];
	float tone_power_1 = y[tone_index_1];
	float tone_power_2 = y[tone_index_2];
	float *ptone_power_1;
	float *ptone_power_2;

	ptone_power_1 =  &tone_power_1;
	ptone_power_2 =  &tone_power_2;
	__set_prec_single();
	//ratio
	float *ppower_ratio;
	float   power_ratio;

	ppower_ratio =  &power_ratio;
	__set_Smode(S0hword, S1straight, S2zeros);
	__set_VRAptr_rS0(_VR0);
	__set_VRAptr_rS1(_VR1);
	__ld_Rx_mem_unaligned(0, ptone_power_1);
	__ld_Rx_mem_unaligned(1, ptone_power_2);
	__rd_S0();
	__rd_S1();
	__rd_S2();
	__rcp();
	__rmad_sau();
	__wr(hlinecplx);
	__st_w_asm(ppower_ratio);
	int log_result3, log_result4, power_ratio_dB;

	__log2(log_result3, power_ratio);
	__log2(log_result4, log_norm);
	power_ratio_dB = (10*(log_result3  << 8)) / (log_result4);
	if (power_ratio_dB <= 0)
	{
		power_ratio_dB = 0;
	}
	axiq_tx_disable();
	dmac_abort(0x1<<DMA_CHANNEL_WR);
	dmac_clear_complete(0x1<<DMA_CHANNEL_WR);
	dmac_clear_event(0x1<<dma_channel_rd);
	dmac_clear_event(0x1<<DMA_CHANNEL_WR);
	mailbox_out_msg_0_MSB = (power_ratio_dB  & 0xFFFF);
	mailbox_out_msg_0_LSB = 0x1;
	host_mbox0_post(MAKEDWORD(mailbox_out_msg_0_MSB, mailbox_out_msg_0_LSB));
#endif
}

#endif
