// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#ifndef NCO_CORRELATOR_H
#define NCO_CORRELATOR_H

#include "vcpu.h"

#include <cstdint>

typedef struct NcoCorrelator {
    uint32_t phase; // index of the next sample
    cfloat32_t sum;
} nco_correlator_t;

// Measures one frequency over dmem_lines DMEM lines (32 samples each) of samples, which must be DMEM line aligned.
// freq_ctrl_word / 2^32 is the frequency as a fraction of the sample rate. dmem_lines must not be 0.
// Zero *state before the first block. A block can be split across several calls.
// Afterwards sum is the DFT of the samples at that frequency: N * tone amplitude, with samples scaled to full scale = 1.0.
void nco_correlate(const cfixed16_t *samples, uint16_t dmem_lines, uint32_t freq_ctrl_word, nco_correlator_t *state);

// Hann window with coherent gain 1 expressed in the frequency domain, so it needs no per sample multiply:
//   w[n] = 1 - cos(2pi*n/N)  =>  sum(w[n] * x[n] * exp(-j*2pi*f*n)) = X(f) - (X(f - 1/N) + X(f + 1/N)) / 2
// terms[] are the correlators of the same samples at frequencies f, f - 1/N and f + 1/N.
// *out = terms[0].sum - (terms[1].sum + terms[2].sum) / 2: N * tone amplitude, with samples scaled to full scale = 1.0.
#define NCO_HANN_TERMS 3
void nco_hann_combine(const nco_correlator_t *terms, cfloat32_t *out);

#endif
