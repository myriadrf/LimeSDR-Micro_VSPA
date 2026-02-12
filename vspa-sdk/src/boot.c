// SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
// Copyright 2019 - 2025   NXP Semiconductors

// =============================================================================
//! @file       boot.c
//! @brief      Boot procedure.
//! @author     NXP Semiconductors.
// =============================================================================

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <vspa/intrinsics.h>

#include "chip.h"
#include "vspa.h"
#include "vcpu.h"
#include "host.h"

void boot(void) {
    // Send boot complete message to host:
    host_mbox0_post(0xF100000000000000ULL);

    // Boot message exchange with host:
    do {
        // Wait for message from host:
        do {
        } while (!host_mbox0_event());

        // Read message from host:
        uint64_t msg = host_mbox0_read();

        // Check message from host:
        uint32_t cid = HIWORD(msg) & 0xFF000000;
        if (cid == 0x70000000) {

            // Reply to host:
            host_mbox0_post(0xF070000000000000ULL);

            // Terminate boot sequence:
            break;
        } else {
            // Reply to host:
            host_mbox0_post(MAKEDWORD(0xF0008000 | (cid >> 8), 0));
        }
    } while (1);
}
