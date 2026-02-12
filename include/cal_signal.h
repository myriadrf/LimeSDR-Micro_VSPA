/* SPDX-License-Identifier: BSD-3-Clause */

/*
 * Copyright 2024 NXP
 */

#ifndef SIGNAL_H_
#define SIGNAL_H_

void gen_nco_single_tone(vspa_complex_fixed16 *buffer) __attribute__((aligned(64)));
void TX_single_tone(void) __attribute__((aligned(64)));
void RX_single_tone_measurement(void);

extern uint32_t TX_SingleT_start_bit_update, RX_SingleT_start_bit_update, RX_SingleT_continue;

extern float y[] _VSPA_VECTOR_ALIGN;
extern vspa_complex_fixed16 *TX_SingleT_buffer;
extern vspa_complex_fixed16 *RX_SingleT_buffer;

#endif /* SIGNAL_H_ */
