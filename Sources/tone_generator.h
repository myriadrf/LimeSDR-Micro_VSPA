#ifndef TONE_GENERATOR_H
#define TONE_GENERATOR_H

#include "vcpu.h"

#include <cstdint>

typedef struct ToneState {
    float amplitude;
    uint32_t phase;
    int32_t fcw; // frequency control word
} tone_state_t;

void gen_nco_single_tone(cfixed16_t *buffer, uint16_t dmem_lines, uint32_t freq_ctrl_word, float amplitude, uint32_t *phase);

#endif
