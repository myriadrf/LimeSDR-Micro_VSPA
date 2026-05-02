// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#include "vspa_state.h"

#include "dmac.h"
#include "dma_common.h"

#include "receiver.h"
#include "l1-trace.h"

#define VSPA_HALF_WORDS(x) (2 * (uint32_t)x)

#define IQFLOOD_OUTBOUND_ADDR 0xB0001000
#define VSPA_DMEM_PROXY_ADDR (IQFLOOD_OUTBOUND_ADDR + g_iqflood_proxy_offset)
static uint32_t g_iqflood_proxy_offset = 0;

vspa_state_t player_state __attribute__((aligned(32), section(".dmem_proxy_tx")));

// address have to be 128-bit aligned
static void DDR_write_VSPA_PROXY(uint32_t DDR_wr_dma_channel, uint32_t DDR_address, uint32_t vsp_address, uint32_t size,
                                 bool trig_interrupt) {
    uint32_t ctrl = DMAC_WR | DDR_wr_dma_channel;
    if (trig_interrupt)
        ctrl |= DMAC_TRIG_IRQ;
    dmac_enable(ctrl, size, DDR_address, vsp_address);
}

#if L1_TRACE
void SendL1Trace() {
    if (player_state.info.l1_trace_offset == 0)
        return;

    if (!dmac_is_available(0x1 << DDR_WR_DMA_CHANNEL_5))
        return;

    const uint32_t src = VSPA_HALF_WORDS(l1_trace_data);
    const uint32_t size = 16 * 128; // VSPA_HALF_WORDS(sizeof(l1_trace_data_t) * L1_TRACE_SIZE);
    const uint32_t dest = IQFLOOD_OUTBOUND_ADDR + player_state.info.l1_trace_offset;

    const uint32_t ctrl = DMAC_WR | DDR_WR_DMA_CHANNEL_5;
    dmac_enable(ctrl, size, dest, src);

    player_state.info.l1_trace_offset = 0;
}
#endif

void dmem_proxy_set_offset(uint32_t addr_offset) {
    g_iqflood_proxy_offset = addr_offset;
    EnqueueProxyUpdate(PROXY_UPDATE_ALL);
}

void EnqueueProxyUpdate(uint32_t flags) { player_state.info.proxy_fetch |= flags; }

void VSPA_PROXY_update(void) {
    if (player_state.info.proxy_fetch == 0)
        return;

#if L1_TRACE
    if (player_state.info.proxy_fetch & PROXY_UPDATE_TRACE)
        SendL1Trace();
#endif

    if (!dmac_is_available(0x1 << DDR_WR_DMA_CHANNEL_5))
        return;

    if (player_state.info.proxy_fetch & PROXY_UPDATE_FLOW)

        DDR_write_VSPA_PROXY(DDR_WR_DMA_CHANNEL_5, VSPA_DMEM_PROXY_ADDR, VSPA_HALF_WORDS(&player_state),
                             VSPA_HALF_WORDS(sizeof(player_state)), player_state.info.proxy_fetch & PROXY_UPDATE_INTERRUPT);

    player_state.info.proxy_fetch = 0;
}
