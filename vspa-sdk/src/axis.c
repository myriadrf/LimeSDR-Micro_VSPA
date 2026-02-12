// SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
// Copyright 20182023 - 2025   NXP Semiconductors

// =============================================================================
//! @file       axis.c
//! @brief      AXI slave interface.
//! @author     NXP Semiconductors.
// =============================================================================

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <vspa/intrinsics.h>

#include "chip.h"
#include "vspa.h"
#include "iohw.h"
#include "vcpu.h"
#include "dmac.h"
#include "axis.h"

uint32_t const axis_flag_addr[VSPA_ATID_CNT] = {
#if defined(__LA1224__)
    BASE_VSPA_FLAG(VSPA_ATID_0), BASE_VSPA_FLAG(VSPA_ATID_1), BASE_VSPA_FLAG(VSPA_ATID_2), BASE_VSPA_FLAG(VSPA_ATID_3),
    BASE_VSPA_FLAG(VSPA_ATID_4), BASE_VSPA_FLAG(VSPA_ATID_5), BASE_VSPA_FLAG(VSPA_ATID_6), BASE_VSPA_FLAG(VSPA_ATID_7)
#elif defined(__LA9310__)
    BASE_VSPA_FLAG(VSPA_ATID_0)
#endif
};

struct axis_flag_mask_t axis_flag_mask __align_axi;
