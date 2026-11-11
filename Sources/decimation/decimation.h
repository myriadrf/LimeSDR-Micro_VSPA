// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#ifndef LIME_VSPA_OVERSAMPLE_H
#define LIME_VSPA_OVERSAMPLE_H

#include "fw_config.h"
#include "vcpu.h"

// Samples count fed into the first cascade on each decimate_work() call (one ADC transfer)
#define DEC_INPUT_COUNT 512

#if MAX_DECIMATION_EXPONENT < 1
#error "MAX_DECIMATION_EXPONENT must allow decimation"
#endif
#if (DEC_INPUT_COUNT & (DEC_INPUT_COUNT - 1)) != 0
#error "DEC_INPUT_COUNT must be a power of 2, so cascade outputs fill whole DMEM lines"
#endif

// Largest exponent of one cascade: decimator kernels write whole DMEM lines, so the cascade must produce at least one
// line from DEC_INPUT_COUNT samples: log2(DEC_INPUT_COUNT / SAMPLES_IN_DMEM_LINE)
#define DEC_INPUT_LINES (DEC_INPUT_COUNT / SAMPLES_IN_DMEM_LINE)
#if DEC_INPUT_LINES >= 256
#define DEC_CASCADE_EXPONENT_LIMIT 8
#elif DEC_INPUT_LINES >= 128
#define DEC_CASCADE_EXPONENT_LIMIT 7
#elif DEC_INPUT_LINES >= 64
#define DEC_CASCADE_EXPONENT_LIMIT 6
#elif DEC_INPUT_LINES >= 32
#define DEC_CASCADE_EXPONENT_LIMIT 5
#elif DEC_INPUT_LINES >= 16
#define DEC_CASCADE_EXPONENT_LIMIT 4
#elif DEC_INPUT_LINES >= 8
#define DEC_CASCADE_EXPONENT_LIMIT 3
#elif DEC_INPUT_LINES >= 4
#define DEC_CASCADE_EXPONENT_LIMIT 2
#elif DEC_INPUT_LINES >= 2
#define DEC_CASCADE_EXPONENT_LIMIT 1
#else
#error "DEC_INPUT_COUNT too small to decimate into whole DMEM lines"
#endif

// One cascade when the maximum exponent fits into it, else as many full cascades as needed, the last one decimates the rest
#if MAX_DECIMATION_EXPONENT <= DEC_CASCADE_EXPONENT_LIMIT
#define DEC_EXPONENT_PER_CASCADE MAX_DECIMATION_EXPONENT
#else
#define DEC_EXPONENT_PER_CASCADE DEC_CASCADE_EXPONENT_LIMIT
#endif
#define DEC_CASCADES_COUNT ((MAX_DECIMATION_EXPONENT + DEC_EXPONENT_PER_CASCADE - 1) / DEC_EXPONENT_PER_CASCADE)
// Each stage within a cascade is a x4 (or the last one a x2) decimator kernel
#define DEC_STAGES_IN_CASCADE ((DEC_EXPONENT_PER_CASCADE + 1) / 2)

#if DEC_CASCADES_COUNT > 1
// A following cascade needs (32 << its exponent) input samples to produce one DMEM line. Its exponent is a full cascade's,
// or the remainder when it is the last one.
#if (MAX_DECIMATION_EXPONENT - DEC_EXPONENT_PER_CASCADE) < DEC_EXPONENT_PER_CASCADE
#define DEC_CASCADE_BUFFER_COUNT (SAMPLES_IN_DMEM_LINE << (MAX_DECIMATION_EXPONENT - DEC_EXPONENT_PER_CASCADE))
#else
#define DEC_CASCADE_BUFFER_COUNT (SAMPLES_IN_DMEM_LINE << DEC_EXPONENT_PER_CASCADE)
#endif
#endif

typedef struct DecimatorCascade {
    // filter state of each x2/x4 stage in the cascade
    cfixed16_t history[DEC_STAGES_IN_CASCADE][SAMPLES_IN_DMEM_LINE] _VSPA_VECTOR_ALIGN;
} decimator_cascade_t;

typedef struct DecimationPipeline {
#if DEC_CASCADES_COUNT > 1
    // output of a cascade accumulated until the following cascade has enough input, processed there in place
    cfixed16_t cascade_output[DEC_CASCADES_COUNT - 1][DEC_CASCADE_BUFFER_COUNT] _VSPA_VECTOR_ALIGN; // don't need clearing
#endif
    decimator_cascade_t cascade[DEC_CASCADES_COUNT];
    uint16_t samples_produced; // valid samples in output buffer
#if DEC_CASCADES_COUNT > 1
    uint16_t cascade_produced[DEC_CASCADES_COUNT - 1]; // valid samples in cascade_output
#endif
    uint16_t decimate_exponent; // copy of the selected channel's config, latched on channel select and lane prime
} decimation_pipeline_t;

extern decimation_pipeline_t dec_pipe[RX_MAX_LANE_COUNT];

static inline void decimation_clear(decimation_pipeline_t *pipe) {
    // struct members specially layout for single memset clearing
    memset16(pipe->cascade, 0, sizeof(pipe->cascade) + sizeof(pipe->samples_produced) * DEC_CASCADES_COUNT);
}

static inline void decimate_output_consume(decimation_pipeline_t *pipe) { pipe->samples_produced = 0; }

// Decimates src_count (DEC_INPUT_COUNT) samples by the pipeline's exponent, src is overwritten (processed in place).
// Appends output to the output buffer and advances samples_produced, which can stay unchanged while
// the second cascade is accumulating input.
void decimate_work(decimation_pipeline_t *pipe, cfixed16_t *dest, cfixed16_t *src, uint16_t src_count);

#endif // LIME_VSPA_OVERSAMPLE_H