// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#ifndef LIME_L1_TRACE_H
#define LIME_L1_TRACE_H

#include <stdint.h>
#include "ccnt.h"

#define TRACE_ENABLED 0
#define DEFAULT_THREAD_ID 1

typedef struct l1_trace_data_s {
    uint64_t cnt; // VSPA has only 48bit counter
    uint32_t msg;
    uint32_t param;
} l1_trace_data_t;

enum {
    T_XFER_BUFFER = 0,
    T_QEC_TX_BUFFER,
    T_QEC_RX_BUFFER,
    T_DEC_BUFFER,
    T_INT_BUFFER,
    T_UNDERRUN,
    T_OVERRUN,
    T_XFER_ERROR,
    T_XFER_CFG_ERROR,
    T_UNEXPECTED,
    T_NO_MEMORY,
    T_AXIQ_ENQ,
    T_DDR_ENQ,
    T_GO,
    T_ADC_ENQ,
    T_DAC,
    T_DDR_RD,
    T_DDR_WR,
    T_HOST_PRODUCE,
    T_DMA_NOT_AVAILABLE,
    T_AXIQ_COMPLETE,
    T_DDR_COMPLETE,
    T_AXIQ_TX_ENABLE,
    T_AXIQ_RX0_ENABLE,
    T_AXIQ_RX1_ENABLE,
    T_AXIQ_RO0_ENABLE,
    T_AXIQ_RO1_ENABLE,
    T_BUFFER_FILL,
    T_INTER_CACHE_FILL,
    T_DEC_CACHE_FILL,
    T_PHYTIMER,
    T_ERROR,
    T_TIME_NOW,
    T_MBOX,
    T_ADC_COMPLETE,
    T_DDR_WR_COMPLETE,
    T_EXTERNAL_GO,
    T_GENERATE_TONE,
};

typedef struct l1_trace_state_s {
    uint32_t la9310_mem_address;
    uint32_t buffer_size;
    uint32_t bytes_produced;
    uint32_t event_count;
    uint32_t event_drops;
} l1_trace_hif_t; // L1 trace host interface

#if TRACE_ENABLED
extern l1_trace_hif_t trace_hif;
extern void l1_trace_init(void);
extern void l1_trace_clear(void);
void l1_trace_upload(void);
void l1_trace(uint32_t msg, uint32_t param);

void l1_trace_duration(uint64_t startcnt, uint32_t msg);

void push_traces();
void check_l1_trace_complete(void);

#else
static inline void l1_trace_init(void) {}
static inline void l1_trace_clear(void) {}
static inline void l1_trace_upload(void) {}
static inline void l1_trace(uint32_t msg, uint32_t param) {}
static inline void l1_trace_duration(uint64_t startcnt, uint32_t msg) {}
static inline void push_traces() {}
static inline void check_l1_trace_complete(void) {}
#endif

enum {
    TG_NONE = 0,
    TG_VCPU = (1 << 0),
    TG_DMA = (2 << 0),
    TG_IPPU = (3 << 0),
};

enum {
    TRACE_MARK_EVENT = 0,
    TRACE_MARK_COUNTER = 1,
    TRACE_MARK_BEGIN = 2,
    TRACE_MARK_END = 3,
    TRACE_MARK_COMPLETE = 4,
};

enum {
    CNT_TX_UDR,
    CNT_TX_OVR,
    CNT_RX0_UDR,
    CNT_RX1_UDR,
    CNT_RX2_UDR,
    CNT_RX3_UDR,
    CNT_RX0_OVR,
    CNT_RX1_OVR,
    CNT_RX2_OVR,
    CNT_RX3_OVR,
    CNT_DECIM,
    CNT_INTERP,
    CNT_PHYTIME,
    CNT_TX_AXIQ_EN,
    CNT_TX_DMA_ALLOW,
    CNT_DDR_RD_ENQ,
    CNT_DDR_RD_READY,
    CNT_ADC_ENQ,
    CNT_DAC_ENQ,
    CNT_DAC_COMPLETION_TIME,
    CNT_DDR_RD_COMPLETION_TIME,
    CNT_DDR_WR_ENQ,
    CNT_ADC_READY,
    CNT_DAC_READY,
    CNT_POOL,
};

#if TRACE_ENABLED

#define TRACE_EVENT(type, id, param)                                                             \
    do {                                                                                         \
        {                                                                                        \
            l1_trace(TG_VCPU << 28 | TRACE_MARK_EVENT << 25 | id << 20 | type, (uint32_t)param); \
        }                                                                                        \
    } while (0)

#define TRACE_BEGIN(type, id, param)                                                             \
    do {                                                                                         \
        {                                                                                        \
            l1_trace(TG_VCPU << 28 | TRACE_MARK_BEGIN << 25 | id << 20 | type, (uint32_t)param); \
        }                                                                                        \
    } while (0)

#define TRACE_END(type, id, param)                                                             \
    do {                                                                                       \
        {                                                                                      \
            l1_trace(TG_VCPU << 28 | TRACE_MARK_END << 25 | id << 20 | type, (uint32_t)param); \
        }                                                                                      \
    } while (0)

#define TRACE_COUNTER(counterId, value)                                                      \
    do {                                                                                     \
        {                                                                                    \
            l1_trace(TG_VCPU << 28 | TRACE_MARK_COUNTER << 25 | counterId, (uint32_t)value); \
        }                                                                                    \
    } while (0)

#define TRACE_DMA_BEGIN(channel, param)                                                                       \
    do {                                                                                                      \
        {                                                                                                     \
            l1_trace(TG_DMA << 28 | TRACE_MARK_BEGIN << 25 | channel << 20 | T_XFER_BUFFER, (uint32_t)param); \
        }                                                                                                     \
    } while (0)

#define TRACE_DMA_END(channel, param)                                                                       \
    do {                                                                                                    \
        {                                                                                                   \
            l1_trace(TG_DMA << 28 | TRACE_MARK_END << 25 | channel << 20 | T_XFER_BUFFER, (uint32_t)param); \
        }                                                                                                   \
    } while (0)

#define TRACE_DURATION(type, id, start_ccnt)                                                            \
    do {                                                                                                \
        {                                                                                               \
            l1_trace_duration(start_ccnt, TG_VCPU << 28 | TRACE_MARK_COMPLETE << 25 | id << 20 | type); \
        }                                                                                               \
    } while (0)

#define TRACE_START_DURATION(x) uint64_t x = ccnt_read();

#else

#define TRACE_EVENT(type, id, param)
#define TRACE_BEGIN(type, id, param)
#define TRACE_END(type, id, param)
#define TRACE_COUNTER(counterId, value)
#define TRACE_DMA_BEGIN(channel, param)
#define TRACE_DMA_END(channel, param)
#define TRACE_DURATION(type, id, start_ccnt)
#define TRACE_START_DURATION(x)

#endif

#endif // LIME_L1_TRACE_H
