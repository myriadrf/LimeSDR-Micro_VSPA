/* SPDX-License-Identifier: BSD-3-Clause */

/*
 * Copyright 2024 NXP
 */

#ifndef IQ_CAL_H_
#define IQ_CAL_H_

void RX_DCOC_CAL(void) __attribute__((aligned(64)));
void TX_DCOC_CAL(void);
void BW_CAL(void);

#endif /* IQ_CAL_H_ */
