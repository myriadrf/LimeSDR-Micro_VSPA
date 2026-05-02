#ifndef TONE_GENERATOR_H
#define TONE_GENERATOR_H

#include <vspa/intrinsics.h>
#include <cstdint>

typedef struct ToneState {
    float amplitude;
    uint32_t phase;
    uint16_t freq_bin;
} tone_state_t;

void gen_nco_single_tone(vspa_complex_fixed16 *buffer, uint32_t samplesCount, tone_state_t *state);

#endif
