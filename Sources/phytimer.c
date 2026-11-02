#include "phytimer.h"

#include <stddef.h>
#include <stdint.h>

#include "chip.h"
#include "host.h"
#include "dmac.h"
#include "phy-timer.h"

#include "dma_common.h"
#include "vcpu.h"

#include "l1-trace.h"
#include "vspa_iqstream.h"

#define PHY_TMR_DMA_CHAN DDR_WR_DMA_CHANNEL_5
#define PHY_TMR_DMA_CHAN_MASK (1 << PHY_TMR_DMA_CHAN)

#define BIT(x) (1 << x)

/* PHY Timer Comparator flags */
/* Clear the interrupt notification bit for this comparator */
// #define PHY_TIMER_COMPARATOR_CLEAR_INT BIT(7)

/* Setting this flag disables this comparator. Enabling a comparator
 * is done by configuring it with a trigger value */
// #define PHY_TIMER_COMPARATOR_DISABLE BIT(6)

/* Setting this flag captures the current timer value as the comparator
 * value */
#define PHY_TIMER_COMPARATOR_CAPTURE BIT(5)

/* Setting this flag enables compare equal to generate a cross-trigger
 * output to VSPA */
#define PHY_TIMER_COMPARATOR_CROSS_TRIG BIT(4)

/* PHY Timer Status */
/* Current status of output signal */
#define PHY_TIMER_COMPARATOR_STATUS_OUT_HIGH BIT(31)

/* If this is 0 compare equal did not occur, value of 1 means compare
 * equal occured */
#define PHY_TIMER_COMPARATOR_STATUS_INT BIT(7)
/* if this is 1, comparator is enabled */
#define PHY_TIMER_COMPARATOR_STATUS_ENABLED BIT(6)
#define PHY_TIMER_COMPARATOR_STATUS_MASK \
    (PHY_TIMER_COMPARATOR_STATUS_OUT_HIGH | PHY_TIMER_COMPARATOR_STATUS_INT | PHY_TIMER_COMPARATOR_STATUS_ENABLED)

// void phy_tmr_enable(void)
// {
//     uint32_t const ctrl = PHY_TMR_CTRL_EN;
//     dmac_enable(DMAC_DI32 |PHY_TMR_DMA_CHAN, 1,
//         2 * (uint32_t)&phy_tmr_ctrl_addr, 2 * (uint32_t)&ctrl);
//     do {/* wait */} while (!dmac_is_complete(PHY_TMR_DMA_CHAN_MASK));
//     dmac_clear_complete(PHY_TMR_DMA_CHAN_MASK);
// }

static uint32_t scratch_buf_addrs[10] = { 0 };
static uint32_t scratch_buf_values[10] = { 0 };

uint32_t timer_trig_immediate_async(uint32_t id, enum ePhyTimerComparatorTrigger trigger) {
    WAIT_FOR(!dmac_is_enabled(PHY_TMR_DMA_CHAN_MASK), VSPA_DEFAULT_TIMEOUT);
    if (dmac_is_enabled(PHY_TMR_DMA_CHAN_MASK))
        return 0;

    dmac_clear_errcfg(PHY_TMR_DMA_CHAN_MASK);
    dmac_clear_complete(PHY_TMR_DMA_CHAN_MASK);
    scratch_buf_addrs[0] = PHY_TMR_SC_ADDR(id);
    scratch_buf_values[0] = PHY_TIMER_COMPARATOR_DISABLE | PHY_TIMER_COMPARATOR_CLEAR_INT;
    scratch_buf_addrs[1] = PHY_TMR_SC_ADDR(id);
    scratch_buf_values[1] = trigger << 2;

    dmac_enable(DMAC_DI32 | PHY_TMR_DMA_CHAN, 2, VSPA_HALF_WORDS(scratch_buf_addrs), VSPA_HALF_WORDS(scratch_buf_values));
    return PHY_TMR_DMA_CHAN_MASK;
}

uint16_t timer_trig_immediate(uint32_t id, enum ePhyTimerComparatorTrigger trigger) {
    uint32_t dmamask = timer_trig_immediate_async(id, trigger);
    if (!dmamask)
        return 0;

    WAIT_FOR(dmac_is_complete(dmamask), VSPA_DEFAULT_TIMEOUT);
    return dmamask;
}

uint16_t timer_trig_schedule_async(uint32_t id, enum ePhyTimerComparatorTrigger trigger, uint32_t timestamp) {
    WAIT_FOR(!dmac_is_enabled(PHY_TMR_DMA_CHAN_MASK), VSPA_DEFAULT_TIMEOUT);
    if (dmac_is_enabled(PHY_TMR_DMA_CHAN_MASK))
        return 0;

    dmac_clear_complete(PHY_TMR_DMA_CHAN_MASK);
    scratch_buf_addrs[0] = PHY_TMR_SC_ADDR(id);
    scratch_buf_addrs[1] = PHY_TMR_SC_ADDR(id);
    scratch_buf_addrs[2] = PHY_TMR_V_ADDR(id);

    scratch_buf_values[0] = PHY_TIMER_COMPARATOR_DISABLE | PHY_TIMER_COMPARATOR_CLEAR_INT;
    scratch_buf_values[1] = trigger;
    scratch_buf_values[2] = timestamp;
    dmac_enable(DMAC_DI32 | PHY_TMR_DMA_CHAN, 3, VSPA_HALF_WORDS(scratch_buf_addrs), VSPA_HALF_WORDS(scratch_buf_values));
    return PHY_TMR_DMA_CHAN_MASK;
}

uint16_t timer_trig_schedule(uint32_t id, enum ePhyTimerComparatorTrigger trigger, uint32_t timestamp) {
    uint32_t dmamask = timer_trig_schedule_async(id, trigger, timestamp);
    if (!dmamask)
        return 0;

    WAIT_FOR(dmac_is_complete(dmamask), VSPA_DEFAULT_TIMEOUT);
    return dmamask;
}

void stream_trig_schedule_async(enum ePhyTimerComparatorTrigger trigger, uint32_t timestamp) {
    WAIT_FOR(!dmac_is_enabled(PHY_TMR_DMA_CHAN_MASK), VSPA_DEFAULT_TIMEOUT);
    if (dmac_is_enabled(PHY_TMR_DMA_CHAN_MASK))
        return;

    dmac_clear_complete(PHY_TMR_DMA_CHAN_MASK);
    scratch_buf_addrs[0] = PHY_TMR_SC_ADDR(1);
    scratch_buf_addrs[1] = PHY_TMR_SC_ADDR(2);
    scratch_buf_addrs[2] = PHY_TMR_SC_ADDR(3);
    scratch_buf_addrs[3] = PHY_TMR_SC_ADDR(4);
    scratch_buf_addrs[4] = PHY_TMR_SC_ADDR(11);

    scratch_buf_values[0] = PHY_TIMER_COMPARATOR_DISABLE | PHY_TIMER_COMPARATOR_CLEAR_INT | trigger;
    scratch_buf_values[1] = scratch_buf_values[0];
    scratch_buf_values[2] = scratch_buf_values[0];
    scratch_buf_values[3] = scratch_buf_values[0];
    scratch_buf_values[4] = scratch_buf_values[0];

    scratch_buf_addrs[5] = PHY_TMR_V_ADDR(1);
    scratch_buf_addrs[6] = PHY_TMR_V_ADDR(2);
    scratch_buf_addrs[7] = PHY_TMR_V_ADDR(3);
    scratch_buf_addrs[8] = PHY_TMR_V_ADDR(4);
    scratch_buf_addrs[9] = PHY_TMR_V_ADDR(11);

    // only do Rx triggers, Tx will be scheduled by host
    scratch_buf_values[5] = timestamp;
    scratch_buf_values[6] = scratch_buf_values[5];
    scratch_buf_values[7] = scratch_buf_values[5];
    scratch_buf_values[8] = scratch_buf_values[5];

    dmac_enable(DMAC_DI32 | PHY_TMR_DMA_CHAN, 9, VSPA_HALF_WORDS(scratch_buf_addrs), VSPA_HALF_WORDS(scratch_buf_values));

    WAIT_FOR(dmac_is_complete(PHY_TMR_DMA_CHAN_MASK), VSPA_DEFAULT_TIMEOUT);
}

void timers_trig_schedule(const uint16_t *timer_ids, const uint16_t *triggers, const uint32_t *phytimes, uint16_t count) {
    WAIT_FOR(!dmac_is_enabled(PHY_TMR_DMA_CHAN_MASK), VSPA_DEFAULT_TIMEOUT);
    if (dmac_is_enabled(PHY_TMR_DMA_CHAN_MASK))
        return;

    dmac_clear_complete(PHY_TMR_DMA_CHAN_MASK);

    uint16_t regCount = 0;
    for (uint16_t i = 0; i < count; ++i) {
        scratch_buf_addrs[regCount] = PHY_TMR_SC_ADDR(timer_ids[i]);
        scratch_buf_values[regCount] = PHY_TIMER_COMPARATOR_DISABLE | PHY_TIMER_COMPARATOR_CLEAR_INT | triggers[i];
        ++regCount;
    }
    for (uint16_t i = 0; i < count; ++i) {
        scratch_buf_addrs[regCount] = PHY_TMR_V_ADDR(timer_ids[i]);
        scratch_buf_values[regCount] = phytimes[i];
        ++regCount;
    }
    dmac_enable(DMAC_DI32 | PHY_TMR_DMA_CHAN, regCount, VSPA_HALF_WORDS(scratch_buf_addrs), VSPA_HALF_WORDS(scratch_buf_values));
    WAIT_FOR(dmac_is_complete(PHY_TMR_DMA_CHAN_MASK), VSPA_DEFAULT_TIMEOUT);
}
