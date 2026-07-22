// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Lime Microsystems

#include "vspa_state.h"

#include "dmac.h"
#include "dma_common.h"

#include "receiver.h"
#include "l1-trace.h"

#define DMA_CHANNEL_FOR_STATE DDR_WR_DMA_CHANNEL_4

#define IQFLOOD_OUTBOUND_ADDR 0xB0001000
#define VSPA_DMEM_PROXY_ADDR (IQFLOOD_OUTBOUND_ADDR)
static uint32_t g_iqflood_proxy_offset = 0;

vspa_state_t player_state __attribute__((aligned(32), section(".dmem_proxy_tx")));

// address have to be 128-bit aligned
static void DDR_write_VSPA_PROXY(uint32_t DDR_wr_dma_channel, uint32_t DDR_address, uint32_t vsp_address, uint32_t size,
                                 bool trig_interrupt) {
    uint32_t ctrl = DMAC_WR | DDR_wr_dma_channel; // | DMAC_TRIG_VCPU;
    if (trig_interrupt)
        ctrl |= DMAC_TRIG_IRQ;
    dmac_enable(ctrl, size, DDR_address, vsp_address);
}

void dmem_proxy_set_offset(uint32_t addr_offset) {
    g_iqflood_proxy_offset = addr_offset;
    EnqueueProxyUpdate(PROXY_UPDATE_ALL);
}

uint32_t sentflags = 0;
void EnqueueProxyUpdate(uint32_t flags) { player_state.info.proxy_fetch |= flags; }

void VSPA_PROXY_complete(void) {
    if (!dmac_is_complete(1 << DMA_CHANNEL_FOR_STATE) && !dmac_errcfg(1 << DMA_CHANNEL_FOR_STATE))
        return;

    dmac_clear_complete(1 << DMA_CHANNEL_FOR_STATE);
    dmac_clear_event(1 << DMA_CHANNEL_FOR_STATE);
    // TRACE_END(TG_DMA, T_XFER_BUFFER, sentflags, DMA_CHANNEL_FOR_STATE);
}

void VSPA_PROXY_update(void) {
    VSPA_PROXY_complete();
    if (dmac_is_enabled(0x1 << DMA_CHANNEL_FOR_STATE))
        return;

    uint32_t pf = player_state.info.proxy_fetch;
    if (pf == 0)
        return;

    if (!dmac_is_available(0x1 << DMA_CHANNEL_FOR_STATE))
        return;

    // dmac_clear_errcfg(1 << DMA_CHANNEL_FOR_STATE);
    uint32_t xfer_size = 0;
    if (player_state.info.proxy_fetch & (PROXY_UPDATE_FLOW | PROXY_UPDATE_INTERRUPT))
        xfer_size = VSPA_HALF_WORDS(sizeof(player_state.data_flow));

    if (player_state.info.proxy_fetch & (PROXY_UPDATE_INTERNALS | PROXY_UPDATE_INFO))
        xfer_size = VSPA_HALF_WORDS(sizeof(player_state));

    if (xfer_size) {
        sentflags = pf;
        // TRACE_BEGIN(TG_DMA, T_XFER_BUFFER, sentflags, DMA_CHANNEL_FOR_STATE);
        DDR_write_VSPA_PROXY(DMA_CHANNEL_FOR_STATE, VSPA_DMEM_PROXY_ADDR, VSPA_HALF_WORDS(&player_state), xfer_size,
                             player_state.info.proxy_fetch & PROXY_UPDATE_INTERRUPT);
    }

    player_state.info.proxy_fetch = 0;
}

void MarkEvent(uint32_t mask) { iowr(VCPU_HOST_FLAGS0, mask, mask); }