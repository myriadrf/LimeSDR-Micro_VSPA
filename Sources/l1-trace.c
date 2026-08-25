// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#include "l1-trace.h"

#include "ccnt.h"
#include "dma_common.h"
#include "dmac.h"

#if TRACE_ENABLED

#define L1_TRACE_CAPACITY (128)

#define L1_TRACE_DMA_CHANNEL DDR_WR_DMA_CHANNEL_4

l1_trace_hif_t trace_hif = { 0, 0, 0, 0, 0 };

// double buffer, fill one while another is DMA tranferred
l1_trace_data_t events_buffer[2][L1_TRACE_CAPACITY] __attribute__((aligned(64), section(".ippu_dmem")));
uint16_t events_buffer_fill[2] = { 0, 0 };
uint16_t events_active_buffer = 0;
uint16_t sentsize = 0;

void l1_trace_init(void) {
    ccnt_disable();
    ccnt_reset();
    ccnt_enable();
    l1_trace_clear();
}

void l1_trace_clear(void) {
    dmac_abort((1 << L1_TRACE_DMA_CHANNEL));

    events_active_buffer = 0;
    events_buffer_fill[0] = 0;
    events_buffer_fill[1] = 0;

    trace_hif.bytes_produced = 0;
    trace_hif.event_count = 0;
    trace_hif.event_drops = 0;
    WAIT_TIMEOUT_R(!dmac_is_running(1 << L1_TRACE_DMA_CHANNEL), 5000);
    // while (dmac_is_running(1 << L1_TRACE_DMA_CHANNEL)){
    // }
    dmac_clear_complete((1 << L1_TRACE_DMA_CHANNEL));
    dmac_clear_event((1 << L1_TRACE_DMA_CHANNEL));
}

void l1_trace_upload(void) {
#if TRACE_ENABLED
    check_l1_trace_complete();
    if (trace_hif.la9310_mem_address == 0) // || trace_hif.buffer_size == 0)
        return;

    const uint32_t xfer_size = (events_buffer_fill[events_active_buffer] * 16);
    if (xfer_size == 0)
        return;

    // only 1 transfer is queued up, wait for complete stop of the dma
    if (dmac_is_enabled((1 << L1_TRACE_DMA_CHANNEL))) {
        if (events_buffer_fill[events_active_buffer] >= L1_TRACE_CAPACITY)
            ++trace_hif.event_drops;
        return;
    }

    const uint32_t ctrl = DMAC_WR | L1_TRACE_DMA_CHANNEL; // | DMAC_TRIG_VCPU;
    const uint32_t dest_addr = trace_hif.la9310_mem_address + (trace_hif.bytes_produced % trace_hif.buffer_size);
    const uint32_t srcaddr = VSPA_HALF_WORDS(&events_buffer[events_active_buffer][0]);
    // TRACE_BEGIN(TG_DMA, T_XFER_BUFFER, 0, L1_TRACE_DMA_CHANNEL);
    sentsize = xfer_size;
    dmac_enable(ctrl, xfer_size, dest_addr, srcaddr);

    // swap active buffer
    events_active_buffer = (events_active_buffer + 1) & 0x1;
    events_buffer_fill[events_active_buffer] = 0;
#endif
}

void push_traces(void) {
#if TRACE_ENABLED
    TRACE_START_DURATION(t1);
    // while(events_buffer_fill[events_active_buffer] & 0x7)
    // {
    //     TRACE_EVENT(T_BUFFER_FILL, 0, events_buffer_fill[events_active_buffer]);
    // }
    // if (events_buffer_fill[events_active_buffer] < L1_TRACE_CAPACITY/2)
    //     return;

    l1_trace_upload();
    events_buffer_fill[events_active_buffer] &= (L1_TRACE_CAPACITY - 1); // in case upload failed, reset current buffer
    TRACE_DURATION(T_BUFFER_FILL, 1, t1);
#endif
}

// ~24 cycles
void l1_trace(uint32_t msg, uint32_t param) {
    events_buffer[events_active_buffer][events_buffer_fill[events_active_buffer]].cnt = ccnt_read(); // ccnt_read itself is 5 cycles
    events_buffer[events_active_buffer][events_buffer_fill[events_active_buffer]].msg = msg;
    events_buffer[events_active_buffer][events_buffer_fill[events_active_buffer]].param = param;
    events_buffer_fill[events_active_buffer] = (events_buffer_fill[events_active_buffer] + 1) & (L1_TRACE_CAPACITY - 1);
}

void l1_trace_duration(uint64_t startcnt, uint32_t msg) {
    events_buffer[events_active_buffer][events_buffer_fill[events_active_buffer]].cnt = startcnt; // ccnt_read itself is 5 cycles
    events_buffer[events_active_buffer][events_buffer_fill[events_active_buffer]].msg = msg;
    events_buffer[events_active_buffer][events_buffer_fill[events_active_buffer]].param = ccnt_read() - startcnt;
    events_buffer_fill[events_active_buffer] = (events_buffer_fill[events_active_buffer] + 1) & (L1_TRACE_CAPACITY - 1);
}

void check_l1_trace_complete(void) {
#if TRACE_ENABLED
    if (!dmac_is_complete((1 << L1_TRACE_DMA_CHANNEL)))
        return;

    dmac_clear_complete((1 << L1_TRACE_DMA_CHANNEL));
    dmac_clear_event((1 << L1_TRACE_DMA_CHANNEL));
    // TRACE_END(TG_DMA, T_XFER_BUFFER, 0, L1_TRACE_DMA_CHANNEL);

    trace_hif.bytes_produced += sentsize;
    // sentsize = 0;
#endif
}

#endif
