// SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
// Copyright 2019 - 2025    NXP

// ---------------------------------------------------------------------------
//! @function		ddc2x_8tap
//! @brief
//! @prototype  	ddc2x_8tap( cfixed16_t *pOut,
//!								cfixed16_t *pIn,
//!								float32_t *pTaps,
//!								cfixed16_t *filtState,
//!        						unsigned int n_samples )
//!
//! @param[in]       pIn   			Starting pointer to input vector. This is the first "new" sample. The first stage begins
//! processing
//!									from pIn applied in modulo manner.
//!									Half-fixed complex. Input buffer can be circular.
//! @param[out]      pOut   		Pointer to output buffer. Half-fixed complex. Must be DMEM line aligned.
//! @param[in]       pTaps			Pointer to decimation filter coefficients
//! @param[in]       filtState		Pointer to history buffer. Seven of thirty-two samples in buffer are used.
//! @param[in]       n_samples 		Number of input samples to be processed
//! @return          Void.
//! @cycle         	 73/169 		256 /1024 input samples
//! @stack         	 0
//!	@notice
// ---------------------------------------------------------------------------
void ddc2x_8tap_old(cfixed16_t *pOut, cfixed16_t *pIn, float32_t *pTaps, cfixed16_t *filtState, unsigned int n_samples);

// ---------------------------------------------------------------------------
//! @function		decimator_2x_8_Taps_asm
// ---------------------------------------------------------------------------
void decimator_2x_8_Taps_asm(cfixed16_t *pOut, cfixed16_t *pIn, float32_t *pTaps, cfixed16_t *filtState, unsigned int n_samples);

// ---------------------------------------------------------------------------
//! @function		decimator_4x_8_Taps_asm
// ---------------------------------------------------------------------------
void decimator_4x_8_Taps_asm(cfixed16_t *pOut, cfixed16_t *pIn, float32_t *pTaps, cfixed16_t *filtState, unsigned int n_samples);
