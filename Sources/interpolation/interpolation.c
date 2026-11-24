// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#include "interpolation.h"

// Filter coefficients, in the order the kernels expect (see vspa-lib/inc/filter.h)
static const float tx_filter_taps_upsampling_x2[32] _VSPA_VECTOR_ALIGN = {
#include "fir_interpolation_x2.txt"
};
// same coefficients as x2, just interleaved multiple instances.
static const float tx_filter_taps_upsampling_x4[128] _VSPA_VECTOR_ALIGN = {
#include "fir_interpolation_x4.txt"
};

interpolation_pipeline_t int_pipe[TX_PIPE_COUNT];

// vspa-lib prototypes
// X2_interp_tap32_filter: input count multiple of 64 samples, at least 128
extern void X2_interp_tap32_filter(__fx16 *output, __fx16 *input, unsigned int num_samples, __fx16 *history, float *filter_taps);
// X4_interp_tap64_filter: input count multiple of 16 samples, at least 16
extern void X4_interp_tap64_filter(__fx16 *output, __fx16 *input, unsigned int num_samples, __fx16 *history, float *filter_taps);

// Interpolates src_count samples by 2^exponent (1..INT_EXPONENT_PER_CASCADE) into dest: x4 stages first, a x2 stage last.
// With two stages the first one writes into scratch (src_count << 2 samples), which must not overlap src or dest.
static void interpolate_cascade(interpolator_cascade_t *cas, cfixed16_t *dest, cfixed16_t *src, uint16_t src_count,
                                uint16_t exponent, cfixed16_t *scratch) {
    uint16_t stage = 0;
    do {
        const uint16_t step_pow2 = (exponent >= 2) ? 2 : 1; // x4 stage while possible, then x2
        exponent -= step_pow2;
        // intermediate stage goes into scratch, last stage puts result into final buffer
        cfixed16_t *const output_buffer = (exponent == 0) ? dest : scratch;
        __fx16 *const history = (__fx16 *)cas->history[stage];

        if (step_pow2 == 2)
            X4_interp_tap64_filter((__fx16 *)output_buffer, (__fx16 *)src, src_count, history,
                                   (float *)tx_filter_taps_upsampling_x4);
        else
            X2_interp_tap32_filter((__fx16 *)output_buffer, (__fx16 *)src, src_count, history,
                                   (float *)tx_filter_taps_upsampling_x2);

        src = output_buffer;
        src_count <<= step_pow2;
        ++stage;
    } while (exponent > 0);
}

// Both kernels are causal 16 taps per output phase: y[n] depends on the stage input x[n-15]..x[n], so a stage's output lags
// the response to its last input sample by 15 input samples.
#define INT_KERNEL_HISTORY 15

uint16_t interpolation_tail_count(uint16_t exponent) {
    // stages in the order interpolate_work() runs them: the first cascade (if any), then the last one; x4 stages first
    uint16_t cascade_exponents[2] = { 0, exponent };
#if INT_CASCADES_COUNT > 1
    if (exponent > INT_EXPONENT_PER_CASCADE) {
        cascade_exponents[0] = exponent - INT_EXPONENT_PER_CASCADE;
        cascade_exponents[1] = INT_EXPONENT_PER_CASCADE;
    }
#endif
    uint16_t tail = 0;
    uint16_t done = 0; // exponent applied before the stage
    for (uint16_t c = 0; c < 2; ++c) {
        uint16_t remaining = cascade_exponents[c];
        while (remaining > 0) {
            const uint16_t step_pow2 = (remaining >= 2) ? 2 : 1;
            tail += INT_KERNEL_HISTORY << (exponent - done); // stage input samples, in final output samples
            done += step_pow2;
            remaining -= step_pow2;
        }
    }
    return tail;
}

void interpolation_setup(interpolation_pipeline_t *pipe, cfixed16_t *work) {
    pipe->cascade_output = work;
    pipe->scratch = work + INT_CASCADE_BUFFER_COUNT;
    interpolation_clear(pipe);
}

void interpolate_work(interpolation_pipeline_t *pipe, uint16_t exponent, cfixed16_t *dest, cfixed16_t *src) {
#if INT_CASCADES_COUNT > 1
    // redundant check, caller must ensure it needs interpolation
    // if (exponent == 0)
    //     return;

    if (exponent <= INT_EXPONENT_PER_CASCADE) {
        interpolate_cascade(&pipe->cascade[0], dest, src, INT_OUTPUT_COUNT >> exponent, exponent, pipe->scratch);
        return;
    }

    if (pipe->cascade_remaining == 0) {
        // The first cascade produces a whole block, which covers the following outputs as well. dest is not written until
        // the last cascade runs, so it holds the first cascade's intermediate stage (at most 128 samples).
        const uint16_t first_exponent = exponent - INT_EXPONENT_PER_CASCADE;
        interpolate_cascade(&pipe->cascade[0], pipe->cascade_output, src, INT_CASCADE_BUFFER_COUNT >> first_exponent,
                            first_exponent, dest);
        pipe->cascade_remaining = INT_CASCADE_BUFFER_COUNT;
    }

    const uint16_t step_count = INT_OUTPUT_COUNT >> INT_EXPONENT_PER_CASCADE;
    cfixed16_t *const block = pipe->cascade_output + (INT_CASCADE_BUFFER_COUNT - pipe->cascade_remaining);
    interpolate_cascade(&pipe->cascade[1], dest, block, step_count, INT_EXPONENT_PER_CASCADE, pipe->scratch);
    pipe->cascade_remaining -= step_count;
#else
    interpolate_cascade(&pipe->cascade[0], dest, src, INT_OUTPUT_COUNT >> exponent, exponent, pipe->scratch);
#endif
}
