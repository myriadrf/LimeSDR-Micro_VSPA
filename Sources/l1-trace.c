// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#include "l1-trace.h"

#include "ccnt.h"
#include "dma_common.h"
#include "dmac.h"
#include "vspa_iqstream.h"

void l1_trace_init(void) {
    ccnt_disable();
    ccnt_reset();
    ccnt_enable();
    l1_trace_clear();
}

#if TRACE_ENABLED

#define L1_TRACE_DMA_CHANNEL DDR_WR_DMA_CHANNEL_4

l1_trace_hif_t trace_hif = { 0, 0, 0, 0, 0 };

// double buffer, fill one while another is DMA tranferred
l1_trace_data_t events_buffer[L1_TRACE_CAPACITY] _VSPA_VECTOR_ALIGN __attribute__((section(".bss.vcpu")));
uint16_t events_active_buffer = 0;
uint16_t next_event_index = 0;
uint16_t trace_bytes_submitted = 0;

uint32_t testcnt = 0;

void l1_trace_clear(void) {
    dmac_abort((1 << L1_TRACE_DMA_CHANNEL));

    events_active_buffer = 0;
    next_event_index = 0;

    trace_hif.bytes_produced = 0;
    trace_hif.event_count = 0;
    trace_hif.event_drops = 0;
    trace_bytes_submitted = 0;
    WAIT_FOR(!dmac_is_running(1 << L1_TRACE_DMA_CHANNEL), 5000);

    dmac_clear_complete((1 << L1_TRACE_DMA_CHANNEL));
    dmac_clear_event((1 << L1_TRACE_DMA_CHANNEL));
    dmac_clear_errcfg((1 << L1_TRACE_DMA_CHANNEL));
    dmac_clear_errxfr((1 << L1_TRACE_DMA_CHANNEL));
}

#if TRACE_ENABLED
static inline void check_l1_trace_complete(void) {
    if (!dmac_is_complete(1 << L1_TRACE_DMA_CHANNEL) || dmac_is_enabled(1 << L1_TRACE_DMA_CHANNEL))
        return;

    dmac_clear_complete(1 << L1_TRACE_DMA_CHANNEL);
    // dmac_clear_event((1 << L1_TRACE_DMA_CHANNEL)); // not used

    trace_hif.bytes_produced = trace_bytes_submitted;
}
#endif

void push_traces(void) {
#if TRACE_ENABLED
    if (trace_hif.la9310_mem_address < 0xA0000000)
        return;
    TRACE_START_DURATION(t1);

    check_l1_trace_complete();

    const uint32_t xfer_size = 16 * (next_event_index - events_active_buffer * L1_TRACE_CAPACITY / 2);

    // avoid doing small transfer at the end of each go event. If GO's were spammed
    // the one event transfer might not complete by the time xfer slot would need to be reused.
    if (xfer_size < (L1_TRACE_CAPACITY / 4) * 16)
        return;

    // only 1 transfer is queued up, wait for complete stop of the dma
    if (dmac_is_enabled(1 << L1_TRACE_DMA_CHANNEL)) {
        ++trace_hif.event_drops;
        next_event_index = events_active_buffer * L1_TRACE_CAPACITY / 2;
        return;
    }

    const uint32_t mem_offset = (trace_bytes_submitted % trace_hif.buffer_size);
    if (mem_offset + xfer_size > trace_hif.buffer_size) // would extend beyond host buffer end
    {
        uint32_t first_half_size = trace_hif.buffer_size - mem_offset;
        dmac_enable(DMAC_WR | L1_TRACE_DMA_CHANNEL, first_half_size, trace_hif.la9310_mem_address + mem_offset,
                    VSPA_HALF_WORDS(&events_buffer[events_active_buffer * L1_TRACE_CAPACITY / 2]));

        uint32_t second_half_size = xfer_size - first_half_size;
        dmac_enable(DMAC_WR | L1_TRACE_DMA_CHANNEL, second_half_size, trace_hif.la9310_mem_address,
                    VSPA_HALF_WORDS(&events_buffer[events_active_buffer * L1_TRACE_CAPACITY / 2]) + first_half_size);
    } else // contiguous region on host side
    {
        dmac_enable(DMAC_WR | L1_TRACE_DMA_CHANNEL, xfer_size, trace_hif.la9310_mem_address + mem_offset,
                    VSPA_HALF_WORDS(&events_buffer[events_active_buffer * L1_TRACE_CAPACITY / 2]));
    }
    trace_bytes_submitted += xfer_size;

    // swap active buffer
    events_active_buffer = (events_active_buffer + 1) & 0x1;
    next_event_index = events_active_buffer * L1_TRACE_CAPACITY / 2;

    TRACE_DURATION(T_TRACE_PUSH, DEFAULT_THREAD_ID, t1);
#endif
}

#endif
