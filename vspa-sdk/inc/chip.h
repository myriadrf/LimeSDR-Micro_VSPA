// SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
// Copyright 20182023 - 2025   NXP Semiconductors

// =============================================================================
//! @file       chip.h
//! @brief      Chip-level global definitions.
//! @author     NXP Semiconductors.
// =============================================================================

#ifndef __CHIP_H__
#define __CHIP_H__

#if defined(__LA1575__)
#include "chip-la1575.h"
#elif defined(__LA1224__)
#include "chip-la1224.h"
#elif defined(__LA9310__)
#include "chip-la9310.h"
#else
#error "Chip not supported!"
#endif

#endif // __CHIP_H__
