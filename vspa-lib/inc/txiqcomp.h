// SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
// Copyright 2017 - 2025   NXP Semiconductors

// ===========================================================================
//! @file            txiqcomp.h
//! @brief           Custom TX IQ Compensation functions
//! @ingroup         GROUP_TXIQCOMP
//!
//! The txiqcomp.h header defines IQ compensation functions for WiFi TX
//!
// ===========================================================================

#ifndef __TXIQCOMP_H__
#define __TXIQCOMP_H__

// -----------------------------------------------------------------------------
//! @defgroup       GROUP_TXIQCOMP TX IQ Compensation
//! @brief          TX IQ Compensation functions library
//!
//! This library contains function prototypes for TX signal conditioning functions
//!   - IQ Compensation function
//!      - txiqcomp(): I/Q compensation for TX
//!
//! @{
// -----------------------------------------------------------------------------

#ifndef __ASSEMBLER__

// ---------------------------------------------------------------------------
//! @brief           TX IQ compensation configuration struct.
// ---------------------------------------------------------------------------
typedef struct {
    vspa_complex_float32 dcOffset;
    float IQImb_ftaps[4];
} structTXIQCompParams;

// ---------------------------------------------------------------------------
//! @brief          Signal conditioning chain with 1x decimation for N = 2560 input samples, and, with 4 taps for I/Q imbalance
//!                 fractional delay compensation filter
//!
//! @param[in]       pIn            Starting pointer to input vector. Half-fixed complex. Must be DMEM line aligned.
//! @param[out]      pOut           Pointer to output buffer. Half-fixed complex. Must be DMEM line aligned.
//! @param[in]       pConfig        Pointer to configuration structure. Must be entirely contained within a DMEM line. The
//! individual fields must be configured as
//!                         dcOffset: Complex DC offset in single precision floating point format
//!                         IQImb_ftaps: Array of real taps in the following order (in ascending order of address)
//!                                     [0 ci2q ci cq]
//!                                     where the taps are applied as shown in the formula below.
//! @param[in]       n_linepairs    Set to N/64 where N is the number of input samples.
//! @return          Void.
//! @cycle           24 + 4*n_linepairs
//! @stack           0
//!
//! This is a TX IQ compensation chain that implements the following
//!                 i'(t) = real(dcOffset) + i(t)*ci
//!                 q'(t) = imag(dcOffset) + q(t)*cq + i(t)*ci2q
//!                 [i(t), q(t)] and [i'(t), q'(t)] are real and imaginary parts of input and output streams.
//!                 ci, cq, c12q are real gain components.
//!
// ---------------------------------------------------------------------------
extern void txiqcomp(vspa_complex_fixed16 const *pIn, // Input data pointer in half-fixed precision
                     vspa_complex_fixed16 *pOut,      // Output data pointer in half-fixed precision
                     structTXIQCompParams *pConfig,   // Structure configuring parameters for the channel
                     unsigned int n_linepairs);

// ---------------------------------------------------------------------------
//! @brief           TX IQ compensation configuration struct (type 2)
// ---------------------------------------------------------------------------
typedef float buffTXIQCompTaps[12] _VSPA_VECTOR_ALIGN;

typedef struct {
    buffTXIQCompTaps IQImb_ftaps;  // IQ imbalance correction taps for phase/amplitude correction and fractional delay correction
    unsigned int IQImb_delay;      // Integer delay of I (In-Phase) tap
    vspa_complex_float32 dcOffset; // DC offset (complex single precision) applied directly with addition (and NOT subtraction)
    vspa_complex_fixed16 *inpCircBuffBase; // Input circular buffer base pointer (complex half-fixed, DMEM aligned)
    unsigned int inpCircBuffSize;          // Input circular buffer size (in half-words)
} structTXIQCompParams2 _VSPA_VECTOR_ALIGN;

// ---------------------------------------------------------------------------
//! @brief           Function to apply run-time a real gain to the IQ correction factors
//!                  thus providing a real gain to the signal. The kernel takes an input
//!                  buffer of 12 real single-precision IQ coefficients, scales them and
//!                  writes them in the IQ configuration structure.
//!
//! @param[in]       tap_p        Pointer to the IQ Imbalance correction taps (real single-precision, DMEM aligned).
//! @param[out]      cfg_p        Pointer to configuration structure. Must be DMEM line aligned.
//! @param[in]       gain         Real gain to be applied (real single precision).
//! @return          void
//! @cycle           12
//! @stack           0
//! @note            The computation can be performed in-place.
// ---------------------------------------------------------------------------
extern void txiqcomp_apply_gain(buffTXIQCompTaps *tap_p, structTXIQCompParams2 *cfg_p, float gain);

// ---------------------------------------------------------------------------
//! @brief           TX IQ compensation chain including:
//!                  - IQ phase and amplitude compensation
//!                  - IQ fractional delay compensation
//!                  - DC offset compensation (DC offset is applied with addition and NOT subtraction)
//!
//! @param[in]       inp_p         Starting pointer to input vector. Half-fixed complex. Must be DMEM line aligned.
//! @param[out]      out_p         Pointer to output buffer. Half-fixed complex. Must be DMEM line aligned.
//! @param[in]       cfg_p         Pointer to configuration structure. Must be DMEM line aligned.
//! @param[in]       num_lines     Number of input/output DMEM lines (1 DMEM line is 32 in/out samples).
//! @return          void
//! @cycle           30 + 6 * num_lines
//! @stack           0
//!
//! This is a TX IQ compensation chain that implements the following
//!                 i'(t) = real(dcOffset) + f1 * i(t-D)
//!                 q'(t) = imag(dcOffset) + f2 * i(t-D) + f4 * sum(h1(k) * q(t-k))
//! where:
//! - f1,f2,f4 are real single precision coefficients
//! - h1(k) is a real FIR filter containing the taps for the fractional delay compensation.
//! - for more details look into the implementation plan.
// ---------------------------------------------------------------------------
extern void txiqcomp_x32chf_5t(vspa_complex_fixed16 *inp_p,  // Input  data pointer (complex half-fixed, DMEM aligned)
                               vspa_complex_fixed16 *out_p,  // Output data pointer (complex half-fixed, DMEM aligned)
                               structTXIQCompParams2 *cfg_p, // Configuration structure pointer
                               unsigned int num_lines);      // Number of input/output DMEM lines (1 DMEM line = 32 in/out samples)

#endif //__ASSEMBLER__
#endif // __TXIQCOMP_H__

//! @} GROUP_TXIQCOMP
