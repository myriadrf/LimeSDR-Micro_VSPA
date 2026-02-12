// SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
// Copyright 2018 - 2025   NXP Semiconductors

// =============================================================================
//! @file       axiq.h
//! @brief      AXI I/Q FIFO driver interface.
//! @author     NXP Semiconductors.
// =============================================================================

#ifndef __AXIQ_H__
#define __AXIQ_H__

#if defined(__LA1575__)
#include "axiq-la1575.h"
#elif defined(__LA1224__)
#include "axiq-la1224.h"
#elif defined(__LA9310__)
#include "axiq-la9310.h"
#else
#error "Chip not supported!"
#endif

#endif // __AXIQ_H__
