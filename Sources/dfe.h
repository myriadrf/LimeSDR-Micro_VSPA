/* SPDX-License-Identifier: BSD-3-Clause */

/*
 * Copyright 2024 NXP
 */

#ifndef DFE_H_
#define DFE_H_

#include "vcpu.h"
#include <stdint.h>
#include "txiqcomp.h"

#include "fw_config.h"
#include "vspa_memorymap.h"

// Per channel QEC parameters, written by host through the features map (VSPA_CHANNEL_RX/TX_QEC_ARRAY)
extern vspa_qec_params_t rx_qec_params[RX_CHANNEL_COUNT] _VSPA_VECTOR_ALIGN;
extern vspa_qec_params_t tx_qec_params[TX_CHANNEL_COUNT] _VSPA_VECTOR_ALIGN;

// Sets all channels' QEC to passthrough
void qec_init(void);

// txiqcomp loads the whole DMEM line holding its config and reads the parameters at fixed offsets from the line start,
// so the config must be DMEM line aligned. The per channel parameters share DMEM lines, so the channel's parameters are
// copied into this line aligned work config before each call. Also gives the kernel a consistent snapshot of them.
extern structTXIQCompParams qec_work_params _VSPA_VECTOR_ALIGN;

static inline structTXIQCompParams *qec_load(const vspa_qec_params_t *params) {
    qec_work_params = *(const structTXIQCompParams *)params;
    return &qec_work_params;
}

// @param channel the channel streamed by the pipe whose samples are corrected
static inline void rx_qec_correction(uint16_t channel, cfixed16_t *dataOut, const cfixed16_t *dataIn, uint16_t dmem_lines_count) {
    txiqcomp((vspa_complex_fixed16 *)dataIn, (vspa_complex_fixed16 *)dataOut, qec_load(&rx_qec_params[channel]), dmem_lines_count);
}

static inline void tx_qec_correction(uint16_t channel, cfixed16_t *dataOut, const cfixed16_t *dataIn, uint16_t dmem_lines_count) {
    txiqcomp((vspa_complex_fixed16 *)dataIn, (vspa_complex_fixed16 *)dataOut, qec_load(&tx_qec_params[channel]), dmem_lines_count);
}

#endif /* DFE_H_ */
