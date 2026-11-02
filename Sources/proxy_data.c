#include "proxy_data.h"

#include "dma_common.h"
#include "l1-trace.h"

#include "chip.h"

#define PROXY_DMA_CHANNEL DDR_WR_DMA_CHANNEL_3

proxy_data_t proxy __attribute__((aligned(64)));
bool proxy_dirty = false;

#define INVALID_ADDRESS ((uint32_t)0xFFFFFFFFu)

static uint32_t proxy_memory_offset = INVALID_ADDRESS;

lime_Result proxy_set_address(uint32_t la9310_mem_addr) {
    if (la9310_mem_addr < BASE_PCIE1_TARGET ||
        (la9310_mem_addr + VSPA_HALF_WORDS(sizeof(proxy_data_t))) > (BASE_PCIE1_TARGET + SIZE_PCIE1_TARGET))
        return lime_Result_OutOfRange;
    proxy_memory_offset = la9310_mem_addr - BASE_PCIE1_TARGET;
    return lime_Result_Success;
}

void proxy_clear(void) { memset16(&proxy, 0, sizeof(proxy)); }

void proxy_post(void) {
    TRACE_START_DURATION(t1);
    if (proxy_memory_offset == INVALID_ADDRESS)
        return;

    if (!dmac_is_available(1 << PROXY_DMA_CHANNEL))
        return;

    dmac_enable(DMAC_WR | PROXY_DMA_CHANNEL, VSPA_HALF_WORDS(sizeof(proxy)),
                BASE_PCIE1_TARGET + (proxy_memory_offset % SIZE_PCIE1_TARGET), VSPA_HALF_WORDS(&proxy));

    proxy_dirty = false;
    TRACE_DURATION(T_PROXY, DEFAULT_THREAD_ID, t1);
}
