/* SPDX-License-Identifier: BSD-3-Clause */

/*
 * Copyright 2024 NXP
 */

#include "dfe.h"

vspa_qec_params_t rx_qec_params[RX_CHANNEL_COUNT];
vspa_qec_params_t tx_qec_params[TX_CHANNEL_COUNT];
structTXIQCompParams qec_work_params DMEM_LINE_HEAD;

static void qec_params_reset(vspa_qec_params_t *params) {
    // ftaps {0, 0, 1, 1}: static IQ imbalance correction is passthrough
    params->dc_offset_i = 0.0;
    params->dc_offset_q = 0.0;
    params->ftaps[0] = 0.0;
    params->ftaps[1] = 0.0;
    params->ftaps[2] = 1.0;
    params->ftaps[3] = 1.0;
}

void qec_init(void) {
    for (uint16_t channel = 0; channel < RX_CHANNEL_COUNT; ++channel)
        qec_params_reset(&rx_qec_params[channel]);
    for (uint16_t channel = 0; channel < TX_CHANNEL_COUNT; ++channel)
        qec_params_reset(&tx_qec_params[channel]);
}
