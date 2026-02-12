// SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
// Copyright 2018 - 2025   NXP Semiconductors

// =============================================================================
//! @file       ovly.c
//! @brief      Overlay management interface.
//! @author     NXP Semiconductors.
// =============================================================================

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <vspa/intrinsics.h>

#include "vspa.h"
#include "vcpu.h"
#include "dmac.h"
#include "pdma.h"

#include "ovly.h"

// Enable variable - bit x of this variables means that overlay ovly_table[x] is loaded and enabled in the user application;
// this is the variable read by the debugger to find out the loaded overlays and refresh accordingly the debug views
// See "AN12074 Using Overlays in CodeWarrior for VSPA" in compiler help folder (./VSPA/Help/PDF/)
uint32_t ovly_mapped = 0;

// Syscall function to be called when changing the overlay and after the ovly_mapped variable is updated
// This is a debugger hook used to perform updates. When this is called the debugger will stop the core and will perform internal
// updates See "AN12074 Using Overlays in CodeWarrior for VSPA" in compiler help folder (./VSPA/Help/PDF/)
void ovly_debug_event(void) { __fnop(); }
