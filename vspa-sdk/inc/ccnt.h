// SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
// Copyright 2018 - 2025   NXP Semiconductors

// =============================================================================
//! @file       ccnt.h
//! @brief      Cycle counting and profiling interface.
//! @author     NXP Semiconductors.
// =============================================================================

#ifndef __CCNT_H__
#define __CCNT_H__

#include "iohw.h"
#include "vcpu.h"

typedef int64_t ccnt_t;

static inline uint32_t ccnt_is_enabled(void) { return iord(CYC_CNTR_MSB, 0b1 << 31); }

static inline void ccnt_disable(void) { iowr(CYC_CNTR_MSB, 0b1 << 31, 0b0 << 31); }

#pragma cplusplus on
static inline void ccnt_enable(void) { iowr(CYC_CNTR_MSB, 0b1 << 31, 0b1 << 31); }

static inline void ccnt_enable(ccnt_t count) { iowr64(CYC_CNTR_MSB, (0x1LL << 63) | count); }
#pragma cplusplus reset

static inline void ccnt_init(ccnt_t count) { iowr64(CYC_CNTR_MSB, count); }

static inline void ccnt_reset(void) { iowr64(CYC_CNTR_MSB, 0x0LL); }

static inline ccnt_t ccnt_read(void) { return MAKEDWORD(iord(CYC_CNTR_MSB, 0x0000FFFF), iord(CYC_CNTR_LSB)); }

static inline ccnt_t ccnt_read_lsb32(void) { return iord(CYC_CNTR_LSB); }

// -----------------------------------------------------------------------------
//! @defgroup   GROUP_CCNT_NOT_NESTED   Non-nested Cycle Count
//! @{
// -----------------------------------------------------------------------------

extern ccnt_t ccnt_offset;

static inline void ccnt_start(void) {
    ccnt_disable();
    ccnt_enable(0ULL);
}

static inline ccnt_t ccnt_stop(void) {
    ccnt_disable();
    return ccnt_read() - ccnt_offset;
}

static inline void ccnt_calibrate(void) {
    ccnt_start();
    ccnt_offset = ccnt_stop();
}

//! @}

// -----------------------------------------------------------------------------
//! @defgroup   GROUP_CCNT_NESTED   Nested Cycle Count
//! @{
// -----------------------------------------------------------------------------

extern ccnt_t ccnt_section_offset;

static inline ccnt_t ccnt_section_start(void) { return ccnt_read(); }

static inline ccnt_t ccnt_section_stop(ccnt_t count) { return ccnt_read() - count - ccnt_section_offset; }

static inline void ccnt_section_calibrate(void) {
    ccnt_t count = ccnt_section_start();
    ccnt_section_offset = ccnt_section_stop(count);
}

//! @}

struct ccnt_stat_t {
    uint32_t max;
    uint32_t min;
    uint32_t avg;
    int ncall;
    ccnt_t ccnt;
};

static inline void ccnt_stat_init(struct ccnt_stat_t *this) {
    this->max = 0;
    this->min = 0x7FFFFFFFFFFFFFFFLL;
    this->avg = 0;
    this->ncall = 1;
}

static inline void ccnt_stat_start(struct ccnt_stat_t *this) { ccnt_start(); }

static inline void ccnt_stat_stop(struct ccnt_stat_t *this) {
    this->ccnt = ccnt_stop();
    this->max = this->ccnt > this->max ? (uint32_t)this->ccnt : this->max;
    this->min = this->ccnt < this->min ? (uint32_t)this->ccnt : this->min;
    this->avg = ((this->avg * this->ncall) + (uint32_t)this->ccnt) / this->ncall;
    this->ncall++;
}

#endif // __CCNT_H__
