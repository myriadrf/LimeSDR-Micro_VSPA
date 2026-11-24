// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#ifndef LIME_VSPA_INTERPOLATION_H
#define LIME_VSPA_INTERPOLATION_H

#include "fw_config.h"
#include "vcpu.h"

#include <stdbool.h>
#include <stdint.h>

// Samples produced on each interpolate_work() call (one DAC transfer)
#define INT_OUTPUT_COUNT 512

// The x2 kernel needs at least 128 input samples, so it is always the last stage of a cascade and produces 256+ samples
#define INT_X2_MIN_OUTPUT_COUNT 256

#if MAX_INTERPOLATION_EXPONENT < 1
#error "MAX_INTERPOLATION_EXPONENT must allow interpolation"
#endif
#if INT_OUTPUT_COUNT < INT_X2_MIN_OUTPUT_COUNT || (INT_OUTPUT_COUNT % (2 * SAMPLES_IN_DMEM_LINE)) != 0
#error "INT_OUTPUT_COUNT must suit the x2 kernel: at least 256 samples, in whole x2 input lines"
#endif

// Largest exponent of one cascade into INT_OUTPUT_COUNT samples. Interpolator kernels read whole DMEM lines, so the
// cascade's input must be at least one line: log2(INT_OUTPUT_COUNT / SAMPLES_IN_DMEM_LINE). interpolate_cascade() also
// runs two stages at most (x4, then x4 or x2), so x16 at most.
#define INT_OUTPUT_LINES (INT_OUTPUT_COUNT / SAMPLES_IN_DMEM_LINE)
#if INT_OUTPUT_LINES >= 16
#define INT_CASCADE_EXPONENT_LIMIT 4
#elif INT_OUTPUT_LINES >= 8
#define INT_CASCADE_EXPONENT_LIMIT 3
#else // INT_OUTPUT_COUNT >= 256 gives at least 8 lines
#error "INT_OUTPUT_COUNT too small for a cascade"
#endif

// One cascade when the maximum exponent fits into it, else a second (input side) cascade interpolates the rest
#if MAX_INTERPOLATION_EXPONENT <= INT_CASCADE_EXPONENT_LIMIT
#define INT_CASCADES_COUNT 1
#define INT_EXPONENT_PER_CASCADE MAX_INTERPOLATION_EXPONENT
#else
#define INT_CASCADES_COUNT 2
#define INT_EXPONENT_PER_CASCADE INT_CASCADE_EXPONENT_LIMIT
#endif
// Each stage within a cascade is a x4 (or the last one a x2) interpolator kernel
#define INT_STAGES_IN_CASCADE ((INT_EXPONENT_PER_CASCADE + 1) / 2)

#if INT_CASCADES_COUNT > 1
// The first cascade produces this many samples per run, and the last cascade consumes them over several outputs. It is the
// smallest block that every first cascade exponent can produce, as its x2 stage needs 128 input samples.
#define INT_CASCADE_BUFFER_COUNT INT_X2_MIN_OUTPUT_COUNT
// The first cascade's input must be whole DMEM lines too, so it can interpolate x8 at most
#if (INT_CASCADE_BUFFER_COUNT >> (MAX_INTERPOLATION_EXPONENT - INT_EXPONENT_PER_CASCADE)) < SAMPLES_IN_DMEM_LINE
#error "MAX_INTERPOLATION_EXPONENT is too big for two cascades"
#endif
#if (INT_CASCADE_BUFFER_COUNT % (INT_OUTPUT_COUNT >> INT_EXPONENT_PER_CASCADE)) != 0
#error "last cascade must consume the first cascade's output block in whole steps"
#endif
#else
#define INT_CASCADE_BUFFER_COUNT 0
#endif

// Output of the intermediate (x4) stage within the last cascade, largest when a x2 stage follows: INT_OUTPUT_COUNT / 2.
// The first cascade's intermediate stage goes into the output buffer instead.
#if INT_STAGES_IN_CASCADE > 1
#define INT_SCRATCH_COUNT (INT_OUTPUT_COUNT / 2)
#else
#define INT_SCRATCH_COUNT 0
#endif

// Work memory needed when interpolation_needs_work_memory()
#define INT_WORK_MEMORY_COUNT (INT_CASCADE_BUFFER_COUNT + INT_SCRATCH_COUNT)

typedef struct InterpolatorCascade {
    // filter state of each x2/x4 stage in the cascade
    cfixed16_t history[INT_STAGES_IN_CASCADE][SAMPLES_IN_DMEM_LINE] _VSPA_VECTOR_ALIGN;
} interpolator_cascade_t;

typedef struct InterpolationPipeline {
    // cascade[0] is the only cascade up to INT_EXPONENT_PER_CASCADE, else it is the first (input side) one
    interpolator_cascade_t cascade[INT_CASCADES_COUNT];
    uint16_t cascade_remaining; // samples of cascade_output not yet consumed by the last cascade
    cfixed16_t *cascade_output; // first cascade output block (INT_CASCADE_BUFFER_COUNT), in work memory
    cfixed16_t *scratch; // output of the intermediate stage within the last cascade (INT_SCRATCH_COUNT), in work memory
} interpolation_pipeline_t;

extern interpolation_pipeline_t int_pipe[TX_PIPE_COUNT];

// More than one kernel stage needs buffers for the intermediate results
static inline bool interpolation_needs_work_memory(uint16_t exponent) { return exponent > 2; }

// Input samples consumed per interpolate_work() call
static inline uint16_t interpolation_input_count(uint16_t exponent) { return INT_OUTPUT_COUNT >> exponent; }

// Output samples, at the interpolated rate, by which the response to an input sample extends past that sample's own
// outputs (2^exponent of them): the filters' history has to be flushed with zero input for this many outputs at a burst's end.
uint16_t interpolation_tail_count(uint16_t exponent);

// Places the work memory and clears the filters' state.
// work: INT_WORK_MEMORY_COUNT samples, DMEM line aligned. Used only when interpolation_needs_work_memory(exponent).
void interpolation_setup(interpolation_pipeline_t *pipe, cfixed16_t *work);

// Clears the filters' state and drops the buffered intermediate samples, for a discontinuity in the input
static inline void interpolation_clear(interpolation_pipeline_t *pipe) {
    // struct member special layout for single memset clearing
    memset16(pipe->cascade, 0, sizeof(pipe->cascade) + sizeof(pipe->cascade_remaining));
}

// Produces INT_OUTPUT_COUNT samples into dest by 2^exponent (1..MAX_INTERPOLATION_EXPONENT), which must stay the same
// until interpolation_clear().
// src points at the input samples of this output: interpolation_input_count() of them, DMEM line aligned. Successive calls
// must pass successive input (src advanced by interpolation_input_count()), and with two cascades the first cascade reads
// ahead up to INT_CASCADE_BUFFER_COUNT >> (exponent - INT_EXPONENT_PER_CASCADE) samples from src, so the input buffer
// must be a whole multiple of that (512 samples DDR buffers are). Call interpolation_clear() when the input jumps.
void interpolate_work(interpolation_pipeline_t *pipe, uint16_t exponent, cfixed16_t *dest, cfixed16_t *src);

#endif // LIME_VSPA_INTERPOLATION_H
