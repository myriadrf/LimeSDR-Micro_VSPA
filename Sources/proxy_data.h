#ifndef LIME_POSTED_STATS_H
#define LIME_POSTED_STATS_H

#include "opstatus.h"

#include <stdint.h>
#include <stdbool.h>

// time non critical data that's periodically posted to host memory
// Host could poll them from it's own memory without needlesly increasing load on AXI bus.
typedef struct Proxy_Data {
    uint32_t rx_dma_done;
    uint32_t tx_dma_done;
    uint16_t tx_udr;
    uint16_t tx_drop;
    uint16_t rx_ovr;
    uint16_t rx_drop;
} proxy_data_t;

extern proxy_data_t proxy __attribute__((aligned(64)));
extern bool proxy_dirty;

lime_Result proxy_set_address(uint32_t addr);
void proxy_clear(void);

#endif // LIME_POSTED_STATS_H
