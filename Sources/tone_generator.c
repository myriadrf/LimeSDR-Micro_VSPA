// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#include "tone_generator.h"
#include "dma_common.h"
#include "vspa.h"
#include <vspa/intrinsics.h>
#include <cstdint>

#define MEM_LINE_SIZE (512 / 32)

#pragma optimization_level 0
void gen_nco_single_tone(cfixed16_t *buffer, uint32_t samplesCount, tone_state_t *state) {
    vspa_complex_float32 gain;
    gain.real = state->amplitude;
    gain.imag = 0;
    vspa_complex_float32 *gain_ptr;
    const uint32_t dmem_line_count = (samplesCount * 4) / DMEM_LINE_SIZE_BYTES;

    __clr_VRA();
    __set_prec(single, single, single, single, half_fixed);
    __set_Smode(S0word, S1nco, S2zeros);
    __set_VRAptr_rV(_VR2);
    __set_VRAptr_rSt(2);
    __set_VRAincr_rV(_VRH);
    __set_range1_rV(2 * _VR, 2 * _VR + _VRH);
    __set_nco(normal, 0x1, 0);
    gain_ptr = &gain;
    __ld_Rx_mem_unaligned(0, gain_ptr);
    uint32_t tone_freq_DAC = ((int32_t)state->freq_bin) << 16;
    tone_freq_DAC = (tone_freq_DAC ^ 0xFFFFFFFF) + 1; // convert to one's complement
    __set_nco_freq(tone_freq_DAC);
    for (uint32_t i = 0; i < dmem_line_count; i++) {
        __set_nco_phase(state->phase);
        __rd_S0();
        __rd_S1();
        __rd_S2();
        __cmad();
        __wr(hlinecplx);
        state->phase += 16;
        __set_nco_phase(state->phase);
        __rd_S0();
        __rd_S1();
        __rd_S2();
        __cmad();
        __wr(hlinecplx);
        state->phase += 16;
        __st_vec((vspa_vector_pair_fixed16 *)buffer + i);
    }
    // generated data is 2's complement, force DMA to do conversions
    return;
}
#pragma optimization_level reset
