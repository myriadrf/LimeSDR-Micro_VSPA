// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#ifndef RECEIVER_H
#define RECEIVER_H

#include <stdint.h>
#include "opstatus.h"

#define RX_NUM_MAX_CHAN 4

typedef enum {
    VSPA_RO0,
    VSPA_RO1,
    VSPA_RX0,
    VSPA_RX1,
} e_rx_channel;

typedef struct RxControl {
    uint16_t ddr_enabled;
    uint16_t generate_tone;
    uint16_t host_flow_control_disable;
} rx_control_t;

// parameters that host needs to know
typedef struct RxConfig {
    uint32_t ddr_base_address;
    uint32_t ddr_size;
    uint32_t ddr_step;
    uint32_t oversample;
} rx_config_t;

void InitializeRx(void);
uint32_t RxChannelSelect(uint32_t index);

void ConfigRxHostFIFO(e_rx_channel index, uint32_t addr, uint32_t size);
lime_Result RxChannelConfigure(e_rx_channel index, uint32_t decimation);
lime_Result RxPrepare();
lime_Result RxDDR_control(e_rx_channel index, uint64_t msg64);
void ProcessRx(void);

void OnADCRead_Completed(e_rx_channel c);
void OnDDRWR_Completed(uint16_t c);

#endif /* IQMOS_RX_H_ */
