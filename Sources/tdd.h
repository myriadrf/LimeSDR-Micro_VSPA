// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#ifndef VSPA_TDD_H
#define VSPA_TDD_H

#include <stdint.h>
#include "phytimer.h"

typedef struct TxTDD_Config {
    int16_t dac_allowed;
    int16_t pa_on : 8;
    int16_t pa_off : 8;
    int16_t rf_sw_on : 8;
    int16_t rf_sw_off : 8;
    uint16_t rf_sw_on_comparator_out : 2;
    uint16_t rf_sw_off_comparator_out : 2;
    uint16_t pa_on_comparator_out : 2;
    uint16_t pa_off_comparator_out : 2;
} tx_tdd_config_t;

void tx_tdd_schedule_start(const tx_tdd_config_t *tdd, uint32_t phytime);
void tx_tdd_schedule_end(const tx_tdd_config_t *tdd, uint32_t phytime, uint32_t rf_switch_phytime);

#endif // VSPA_TDD_H
