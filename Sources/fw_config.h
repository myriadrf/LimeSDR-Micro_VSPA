#ifndef LIME_VSPA_FW_CONFIG_H
#define LIME_VSPA_FW_CONFIG_H

#include <stdint.h>

#define RX_CHANNEL_COUNT 4
#define RX_PIPE_COUNT 1
#define MAX_DECIMATION_EXPONENT 7

#define TX_CHANNEL_COUNT 1
#define TX_PIPE_COUNT 1
#define MAX_INTERPOLATION_EXPONENT 7

#define SAMPLES_IN_DMEM_LINE 32

// DMEM placement, see the .data section of LCF/la9310_iqplayer.lcf. The linker places objects in input order and does
// not sort them by alignment, so a DMEM line aligned object placed after unaligned data pads the rest of that line.
// Line aligned objects therefore go to dedicated sections that the LCF places before the unaligned data:
// - DMEM_LINES(_CONST): line aligned and a whole number of DMEM lines in size (a struct holding a line aligned member
//   always is). These are packed back to back without padding.
// - DMEM_LINE_HEAD(_CONST): line aligned but smaller than a line. Placed right after the DMEM_LINES(_CONST) objects, so
//   it starts a line, and the unaligned data placed after it fills the rest of that line. Only one object per section
//   comes for free, each further one pads the line before it.
// - DMEM_LINE_TAIL: a second line aligned object smaller than a line. Placed after all other data, so it only pads the
//   line before it.
#define DMEM_LINES __attribute__((section(".bss.dmem_lines"))) _VSPA_VECTOR_ALIGN
#define DMEM_LINES_CONST __attribute__((section(".rom.dmem_lines"))) _VSPA_VECTOR_ALIGN
#define DMEM_LINE_HEAD __attribute__((section(".bss.dmem_line_head"))) _VSPA_VECTOR_ALIGN
#define DMEM_LINE_HEAD_CONST __attribute__((section(".rom.dmem_line_head"))) _VSPA_VECTOR_ALIGN
#define DMEM_LINE_TAIL __attribute__((section(".bss.dmem_line_tail"))) _VSPA_VECTOR_ALIGN

#endif // LIME_VSPA_FW_CONFIG_H
