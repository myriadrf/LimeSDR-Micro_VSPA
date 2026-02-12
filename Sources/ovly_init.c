/* SPDX-License-Identifier: BSD-3-Clause */

/*
 * Copyright 2024 NXP
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <vspa/intrinsics.h>

#include "chip.h"
#include "vspa.h"
#include "vcpu.h"
#include "iohw.h"
#include "dmac.h"
#include "host.h"
#include "ovly.h"
#include "ovly_init.h"
#include "la9310.h"

/* overlay - helper symbol as per "AN12074 Using Overlays in CodeWarrior for VSPA.pdf" */

extern unsigned int _load_address__IQ_data_ovl_ddr;
extern unsigned int _run_address__IQ_data_ovl_ddr;
extern unsigned int _image_length__IQ_data_ovl_ddr;
extern unsigned int _load_address__CAL_ovl_ddr;
extern unsigned int _run_address__CAL_ovl_ddr;
extern unsigned int _image_length__CAL_ovl_ddr;

// Define overlay table with addresses and sizes:
__attribute__((section(".data"))) struct ovly_t ovly_table[REDCAP_MAX_OVLY_CNT];

// Enable variable - bit x of this variables means that overlay ovly_table[x] is loaded and enabled in the user application;
// this is the variable read by the debugger to find out the loaded overlays and refresh accordingly the debug views
// See "AN12074 Using Overlays in CodeWarrior for VSPA" in compiler help folder (./VSPA/Help/PDF/)
__attribute__((section(".data"))) uint32_t ovly_mapped = 0;

// Syscall function to be called when changing the overlay and after the ovly_mapped variable is updated
// This is a debugger hook used to perform updates. When this is called the debugger will stop the core and will perform internal
// updates See "AN12074 Using Overlays in CodeWarrior for VSPA" in compiler help folder (./VSPA/Help/PDF/)
void ovly_debug_event(void) { __fnop(); }

void initOverlays(void) {

    ovly_mapped = REDCAP_OVLY_MAP_CODE_IQ_DATA;
    struct ovly_t *p = &ovly_table[REDCAP_OVLY_CODE_IQ_DATA];

    p->lma = (unsigned int)&_load_address__IQ_data_ovl_ddr << 1;
    p->rma = (unsigned int)&_run_address__IQ_data_ovl_ddr << 1;
    p->size = (unsigned int)&_image_length__IQ_data_ovl_ddr << 1;

    p = &ovly_table[REDCAP_OVLY_CODE_CALIBRATION];
    p->lma = (unsigned int)&_load_address__CAL_ovl_ddr << 1;
    p->rma = (unsigned int)&_run_address__CAL_ovl_ddr << 1;
    p->size = (unsigned int)&_image_length__CAL_ovl_ddr << 1;
}

void redcap_load_ovly(uint32_t ovly_type) {
    // Perform overlay only if not already loaded
    if (ovly_is_mapped(0b1 << ovly_type) != (0b1 << ovly_type)) {
        ovly_load_code(ovly_type, LA9310_DMA_CHAN_OVERLAY); // No vcpu go
        ovly_map(0b1 << ovly_type);
        // Guarantee DATA overlay fully loaded before exiting
        do {
        } while (dmac_is_enabled(LA9310_DMA_MASK_OVERLAY));
        dmac_clear_complete(LA9310_DMA_MASK_OVERLAY);
    }

    // all overlays are done or 'in progress'... tell debugger
    ovly_debug_event();
}
