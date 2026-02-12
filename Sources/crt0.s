// SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
// Copyright 2013 - 2025   NXP Semiconductors


//============================================================================
// Subroutine name: __start
//
// General Description: Start VSP and setup C environment.
//
// This function initializes the stack pointer and the control code switch,
// then jumps to main.
//
//============================================================================
// Subroutines called by this subroutine:
//    (1)  NONE
//============================================================================
//
// Inputs:
//    Register inputs:
//       NONE
//    Memory inputs:
//       NONE
//    IP register inputs:
//       NONE
//    GP register inputs:
//       NONE
// Outputs:
//    Register outputs:
//       NONE
//    Memory outputs:
//       NONE
//    IP register outputs:
//       NONE
//    GP register outputs:
//       NONE
//
// Registers modified: NONE
//
// RAGs modified: NONE
//
// MAGs modified: as15
//
// Storage MAGs modified:  NONE
//
// Persistent memory:
//       NONE
// Scratch memory:
//       NONE
// No. of processing cycles:
//       min:     12.
//       max:     12.
//       typical: 12.
//
// Code size (PRAM/PROM lines): 9.
//
// Notes:
//
//============================================================================
	.section	.init,"ax",@progbits
	.global		_start
	.type		_start, @function
_start
	set.creg	22, 0;				/* Disable VSPA1 mode / enable half-word addressing */
	mv          sp, __stack;    // Initialize stack pointer.
	
	mv		 	a0, sp;              /* clear stack */
    mv		 	g0, ___stack_size/2;
    set.loop 	g0, 1;
    mv		 	g2, 0xdeadbeef;
    st       	[a0]+2, g2;	loop_begin; loop_end;
	
    jsr      _init_bss;           /* Initialize bss to 0 */
    nop;
    nop;
	
	set.creg      4, 0;         // Disable condition code update.
	set.creg     12, 0;         // Disable H capture and increment.
	set.creg     15, 0;         // Disable fft_mode
	set.creg     16, 0;         // Reset fractional interpolation numerator.
	set.creg     17, 0;         // Reset fractional interpolation denominator
	set.creg     18, 0;         // Reset fractional interpolation phase.
	set.creg     20, 0;         // AU output lane [4095:0] in single-precision mode.
	set.creg    255, 0;         // Real mode, full data path, normal AU output.
	mv          a15, 0;        // To disable modulo addressing.
	jmp         _main;          // Jump to C entry-point.
	set.range   a2, a15, 0;    // Disable modulo addressing.
	set.range   a3, a15, 0;    // Disable modulo addressing.
	done;

	.size		_start, .-_start
FuncEnd_start
	
	
   /* Initialize the bss with zeros */
    .section .text
    .global  _init_bss
    .type    _init_bss, @function
_init_bss
    mv       a0, __bss_start;
	mv       g0, __bss_end - __bss_start
	set.loop g0, 1
    mv       g2, 0;
    sth       [a0]+1, g2; loop_begin; loop_end;
    rts;
    fnop;
    fnop;
    .size    _init_bss, .-_init_bss
	
	

