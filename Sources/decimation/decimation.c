// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#include "decimation.h"

// Filter coefficients, must be gain normalized
static const float rx_filter_taps_downsampling[8] _VSPA_VECTOR_ALIGN = {
#include "fir_decimation_x2_x4.txt"
};

decimation_pipeline_t dec_pipe[RX_PIPE_COUNT];

// ddc2x4x.sx prototypes
extern void decimator_2x_8_Taps_asm(cfixed16_t *pOut, volatile cfixed16_t *pIn, const float32_t *pTaps, cfixed16_t *filtState,
                                    uint32_t n_samples);
extern void decimator_4x_8_Taps_asm(cfixed16_t *pOut, volatile cfixed16_t *pIn, const float32_t *pTaps, cfixed16_t *filtState,
                                    uint32_t n_samples);

/// Decimation output must be DMEM aligned (32 samples), so that limits maximum decimation factor to x16 (from 512 samples)
void decimate_cascade(decimator_cascade_t *dec, cfixed16_t *dest, cfixed16_t *src, uint16_t src_count, uint16_t decimate_exponent) {
    uint16_t stage = 0;
    do {
        const uint16_t step_pow2 = (decimate_exponent >= 2) ? 2 : 1; // x4 stage while possible, then x2
        decimate_exponent -= step_pow2;
        // intermediate stages run in place, last stage puts result into final buffer
        cfixed16_t *const output_buffer = (decimate_exponent == 0) ? dest : src;
        cfixed16_t *const history = dec->history[stage];

        if (step_pow2 == 2)
            decimator_4x_8_Taps_asm(output_buffer, src, rx_filter_taps_downsampling, history, src_count);
        else
            decimator_2x_8_Taps_asm(output_buffer, src, rx_filter_taps_downsampling, history, src_count);

        src_count >>= step_pow2;
        ++stage;
    } while (decimate_exponent > 0);
}

void decimate_work(decimation_pipeline_t *pipe, uint16_t exponent, cfixed16_t *dest, cfixed16_t *src, uint16_t src_count) {
#if DEC_CASCADES_COUNT > 1
    // redundant check, caller must ensure that it needs decimation
    // if (exponent == 0)
    //     return;
    for (uint16_t cas = 0;; ++cas) {
        const uint16_t step = (exponent > DEC_EXPONENT_PER_CASCADE) ? DEC_EXPONENT_PER_CASCADE : exponent;
        exponent -= step;
        const uint16_t produced = src_count >> step;

        if (exponent == 0) {
            // last cascade puts result into final buffer
            decimate_cascade(&pipe->cascade[cas], dest, src, src_count, step);
            pipe->samples_produced += produced;
            return;
        }

        // intermediate cascade accumulates its output, until the following cascade can produce a whole DMEM line
        cfixed16_t *const accumulated = pipe->cascade_output[cas];
        decimate_cascade(&pipe->cascade[cas], accumulated + pipe->cascade_produced[cas], src, src_count, step);
        pipe->cascade_produced[cas] += produced;

        const uint16_t next_step = (exponent > DEC_EXPONENT_PER_CASCADE) ? DEC_EXPONENT_PER_CASCADE : exponent;
        if (pipe->cascade_produced[cas] < (SAMPLES_IN_DMEM_LINE << next_step))
            return;

        // following cascade decimates the accumulated samples in place
        src = accumulated;
        src_count = pipe->cascade_produced[cas];
        pipe->cascade_produced[cas] = 0;
    }
#else
    decimate_cascade(&pipe->cascade[0], dest, src, src_count, exponent);
    pipe->samples_produced += src_count >> exponent;
#endif
}
