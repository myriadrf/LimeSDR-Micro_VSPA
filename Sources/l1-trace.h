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
    T_INVALID_VALUE = 0,
    T_BUSY,
    T_GO,
    T_XFER_BUFFER,
    T_EXTERNAL_GO,
    T_TRACE_PUSH,
    T_DDR_WR_ENQ,
    T_DDR_RD_ENQ,
    T_DDR_WR_COMPLETE,
    T_DDR_RD_COMPLETE,
    T_ADC_COMPLETE,
    T_DAC_COMPLETE,
    T_RX_WORK,
    T_TX_WORK,
    T_ADC_ENQ,
    T_DAC_ENQ,
    T_DAC_AXIQ_RST,
    T_ADC_AXIQ_RST,
    T_TX_BURST_START,
    T_TX_BURST_DEFFER,
    T_PHYTIME,
    T_MBOX,
    T_PROXY,
    T_META,
};

typedef struct l1_trace_state_s {
    uint32_t la9310_mem_address;
    uint32_t buffer_size;
    uint32_t bytes_produced;
    uint32_t event_count;
    uint32_t event_drops;
} l1_trace_hif_t; // L1 trace host interface

extern void l1_trace_init(void);

#if TRACE_ENABLED
#define L1_TRACE_CAPACITY (128)
#define L1_TRACE_CAPACITY_MASK (L1_TRACE_CAPACITY - 1)
extern l1_trace_data_t events_buffer[L1_TRACE_CAPACITY] _VSPA_VECTOR_ALIGN __attribute__((section(".bss.vcpu")));
extern l1_trace_hif_t trace_hif;
extern uint16_t next_event_index;
extern void l1_trace_clear(void);

void l1_trace_upload(void);

static inline void l1_trace(uint32_t msg, uint32_t param) {
    events_buffer[next_event_index].cnt = ccnt_read();
    events_buffer[next_event_index].msg = msg;
    events_buffer[next_event_index].param = param;
    next_event_index = (next_event_index + 1) & L1_TRACE_CAPACITY_MASK;
}

static inline void l1_trace_duration(uint64_t startcnt, uint32_t msg) {
    events_buffer[next_event_index].cnt = startcnt;
    events_buffer[next_event_index].msg = msg;
    events_buffer[next_event_index].param = ccnt_read() - startcnt;
    next_event_index = (next_event_index + 1) & L1_TRACE_CAPACITY_MASK;
}

void push_traces();
void check_l1_trace_complete(void);

#else
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
    CNT_GO,
    CNT_HOST_UDR,
    CNT_TX_DFE_UDR,
    CNT_TX_AFE_UDR,
    CNT_TX_AFE_OVR,
    CNT_RX_DDR_ENQ,
    CNT_TX_DDR_ENQ,
    CNT_TX_TCD,
    CNT_TX_DIFF,
    CNT_TX_DMA_ALLOWED,
    CNT_PHYTIME,
    CNT_ADC_ENQ,
    CNT_DAC_ENQ,
    CNT_DAC_PHYTIME_ADVANCE,
    CNT_TX_DDR_READY,
    CNT_DDR_ENQ_ADVANCE,
    CNT_DDR_CMPL_ADVANCE,
    CNT_DEFFER,
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
