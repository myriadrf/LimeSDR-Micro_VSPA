// SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
// Copyright 2022 - 2025  NXP

#ifndef X2_X4_INTERP_FILTERS_H_
#define X2_X4_INTERP_FILTERS_H_

/************************************************************************************
This filter implements the following algorithm (4x upsampling)
input x[n], n=0,1,...,(m*64-1).  number of input samples must be multiple of 64 samples, such as 2048, 2112 etc.
filter h[k], k=0,1,...,15 or 31
output y[n], n=0,1,...,(m*2*64-1).  number of output samples is double of number of input samples
y[n*2] = sigma(x[n-k]*h[k*2]),    k=0,1,...,15. n=0,1,...,(m*64-1)
y[n*2+1] = sigma(x[n-k]*h[k*2+1]),    k=0,1,...,15. n=0,1,...,(m*64-1)

example:
y[0] = x[-15]*h[30] + h[-14]*h[28] +...+ x[-1]*h[2] + x[0]*h[0]
y[1] = x[-15]*h[31] + h[-14]*h[29] +...+ x[-1]*h[3] + x[0]*h[1]
y[2] = x[-14]*h[30] + h[-13]*h[28] +...+ x[0]*h[2] + x[1]*h[0]
y[3] = x[-14]*h[31] + h[-13]*h[29] +...+ x[0]*h[3] + x[1]*h[1]
...


*************************************************************************************/

/*
API: void filter_name(__fx16 *output , __fx16 *input, unsigned int num_samples, __fx16* history, float* filter_taps);
        filter_name: any of the following filters.
        output:      the output samples buffer, aligned to 128 bytes
        input:       the input samples buffer, aligned to 128 bytes
        history:     history buffer, aligned to 128 bytes, must be in size defined by SIZE_HISTORY, should be initialized to 0
        filter_taps: filter taps coefficient buffer, aligned to 128 bytes, must be in size defined by any of the following Macros
for example SIZE_X4_INTERP_TAP64_FILTER_TAPS. num_samples: num samples of the input. requirement may be diffferent for different
filters: For X2_interp_tap16_filter: multiple of 64 samples, minimum 128 samples For X2_interp_tap32_filter: multiple of 64 samples,
minimum 128 samples For X4_interp_tap32_filter: multiple of 16 samples, minimum 32 samples For X4_interp_tap48_filter: multiple of
16 samples, minimum 32 samples For X4_interp_tap64_filter: multiple of 16 samples, minimum 16 samples For X4_interp_tap128_filter:
multiple of 16 samples, minimum 16 samples For fir_filter_16taps: multiple of 128 samples, minimum 128 samples For
fir_filter_31taps: multiple of 128 samples, minimum 128 samples For fir_filter_63taps: multiple of 128 samples, minimum 128 samples
*/

#define SIZE_X2_X4_FILTER_HISTORY 128
#define SIZE_FIR_TAP16_31_HISTORY 128
#define SIZE_FIR_TAP63_HISTORY 256

#define SIZE_X2_INTERP_TAP16_FILTER_TAPS (ALIGN128(16 * 4))
#define SIZE_X2_INTERP_TAP32_FILTER_TAPS (32 * 4)
#define SIZE_X4_INTERP_TAP32_FILTER_TAPS (32 * 2 * 4) // X4 filter taps repeat twice for each tap
#define SIZE_X4_INTERP_TAP48_FILTER_TAPS (48 * 2 * 4)
#define SIZE_X4_INTERP_TAP64_FILTER_TAPS (64 * 2 * 4)
#define SIZE_X4_INTERP_TAP128_FILTER_TAPS (128 * 2 * 4)
#define SIZE_FIR_TAP16_FILTER_TAPS (16 * 4)
#define SIZE_FIR_TAP31_FILTER_TAPS (31 * 4)
#define SIZE_FIR_TAP63_FILTER_TAPS (63 * 4)

#ifndef X2_INTERP_FILTER_DEF_ONLY
void X2_interp_tap16_filter(__fx16 *output, __fx16 *input, unsigned int num_samples, __fx16 *history, float *filter_taps);
void X2_interp_tap32_filter(__fx16 *output, __fx16 *input, unsigned int num_samples, __fx16 *history, float *filter_taps);
void X4_interp_tap32_filter(__fx16 *output, __fx16 *input, unsigned int num_samples, __fx16 *history, float *filter_taps);
void X4_interp_tap48_filter(__fx16 *output, __fx16 *input, unsigned int num_samples, __fx16 *history, float *filter_taps);
void X4_interp_tap64_filter(__fx16 *output, __fx16 *input, unsigned int num_samples, __fx16 *history, float *filter_taps);
void X4_interp_tap128_filter(__fx16 *output, __fx16 *input, unsigned int num_samples, __fx16 *history, float *filter_taps);
void fir_filter_16taps(__fx16 *output, __fx16 *input, unsigned int num_samples, __fx16 *history, float *filter_taps);
void fir_filter_31taps(__fx16 *output, __fx16 *input, unsigned int num_samples, __fx16 *history, float *filter_taps);
void fir_filter_63taps(__fx16 *output, __fx16 *input, unsigned int num_samples, __fx16 *history, float *filter_taps);
void fir_filter_63taps_circ(__fx16 *output, __fx16 *input, unsigned int num_samples, float *filter_taps, void *circ_base,
                            unsigned int circ_size_byte);
#endif

#endif
