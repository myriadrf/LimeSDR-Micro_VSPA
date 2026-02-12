/* SPDX-License-Identifier: BSD-3-Clause */

/*
 * Copyright 2024 NXP
 */

#ifndef OVERLAY_INIT_H_
#define OVERLAY_INIT_H_

//! @brief      Enumeration of overlays.
enum redcap_ovly_t { REDCAP_OVLY_CODE_IQ_DATA, REDCAP_OVLY_CODE_CALIBRATION, REDCAP_MAX_OVLY_CNT };

#define REDCAP_OVLY_MAP_CODE_IQ_DATA (0b1 << REDCAP_OVLY_CODE_IQ_DATA)
#define REDCAP_OVLY_MAP_CODE_CALIBRATION (0b1 << REDCAP_OVLY_CODE_CALIBRATION)

void initOverlays(void);

void redcap_load_ovly(uint32_t ovly_type);

#endif /* OVERLAY_INIT_H_ */
