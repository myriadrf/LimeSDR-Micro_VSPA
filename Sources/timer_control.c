#include "timer_control.h"

#include <stddef.h>
#include <stdint.h>

#include "chip.h"
#include "host.h"
#include "dmac.h"
#include "phy-timer.h"

#include "dma_common.h"
#include "vcpu.h"

#include "l1-trace.h"

#define PHY_TMR_DMA_CHAN DDR_WR_DMA_CHANNEL_5
#define PHY_TMR_DMA_CHAN_MASK (1 << PHY_TMR_DMA_CHAN)

#define BIT(x) (1 << x)

/* PHY Timer Comparator flags */
/* Clear the interrupt notification bit for this comparator */
#define PHY_TIMER_COMPARATOR_CLEAR_INT BIT(7)

/* Setting this flag disables this comparator. Enabling a comparator
 * is done by configuring it with a trigger value */
#define PHY_TIMER_COMPARATOR_DISABLE BIT(6)

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

#define PHY_TMR_SC_ADDR(id) (PHY_TMR_C0SC + (id * 0x8)) // PHY Timer comparator status and control register address.
#define PHY_TMR_V_ADDR(id) (PHY_TMR_C0V + (id * 0x8)) // PHY Timer comparator status and control register address.

static uint32_t scratch_buf_addrs[6] = { 0 };
static uint32_t scratch_buf_values[6] = { 0 };

uint32_t timer_trig_immediate_async(uint32_t id, enum ePhyTimerComparatorTrigger trigger) {
    if (!dmac_is_available(PHY_TMR_DMA_CHAN_MASK))
        return 0;

    dmac_clear_complete(PHY_TMR_DMA_CHAN_MASK);
    scratch_buf_addrs[0] = PHY_TMR_SC_ADDR(id);
    scratch_buf_values[0] = PHY_TIMER_COMPARATOR_DISABLE | PHY_TIMER_COMPARATOR_CLEAR_INT;
    scratch_buf_addrs[1] = PHY_TMR_SC_ADDR(id);
    scratch_buf_values[1] = trigger << 2;

    TRACE_EVENT(T_PHYTIMER, id, trigger);
    // TRACE_BEGIN(TG_DMA, T_XFER_BUFFER, 0, PHY_TMR_DMA_CHAN);
    dmac_enable(DMAC_DI32 | PHY_TMR_DMA_CHAN, 2, VSPA_HALF_WORDS(scratch_buf_addrs), VSPA_HALF_WORDS(scratch_buf_values));
    return PHY_TMR_DMA_CHAN_MASK;
}

uint16_t timer_trig_immediate(uint32_t id, enum ePhyTimerComparatorTrigger trigger) {
    uint32_t dmamask = timer_trig_immediate_async(id, trigger);
    if (!dmamask)
        return 0;

    wait_for_dma(dmamask);
    return dmamask;
}

uint16_t timer_trig_schedule_async(uint32_t id, enum ePhyTimerComparatorTrigger trigger, uint32_t timestamp) {
    if (!dmac_is_available(PHY_TMR_DMA_CHAN_MASK))
        return 0;

    dmac_clear_complete(PHY_TMR_DMA_CHAN_MASK);
    scratch_buf_addrs[0] = PHY_TMR_SC_ADDR(id);
    scratch_buf_addrs[1] = PHY_TMR_SC_ADDR(id);
    scratch_buf_addrs[2] = PHY_TMR_V_ADDR(id);

    scratch_buf_values[0] = PHY_TIMER_COMPARATOR_DISABLE | PHY_TIMER_COMPARATOR_CLEAR_INT;
    scratch_buf_values[1] = trigger;
    scratch_buf_values[2] = timestamp;
    TRACE_EVENT(T_PHYTIMER, id, timestamp);
    dmac_enable(DMAC_DI32 | PHY_TMR_DMA_CHAN, 3, VSPA_HALF_WORDS(scratch_buf_addrs), VSPA_HALF_WORDS(scratch_buf_values));
    return PHY_TMR_DMA_CHAN_MASK;
}

uint16_t timer_trig_schedule(uint32_t id, enum ePhyTimerComparatorTrigger trigger, uint32_t timestamp) {
    uint32_t dmamask = timer_trig_schedule_async(id, trigger, timestamp);
    if (!dmamask)
        return 0;

    wait_for_dma(dmamask);
    return dmamask;
}

uint16_t timer_trig_tx_end_async(uint32_t timestamp) {
    if (!dmac_is_available(PHY_TMR_DMA_CHAN_MASK))
        return 0;

    dmac_clear_complete(PHY_TMR_DMA_CHAN_MASK);
    scratch_buf_addrs[0] = PHY_TMR_SC_ADDR(11);
    scratch_buf_addrs[1] = PHY_TMR_SC_ADDR(11);
    scratch_buf_addrs[2] = PHY_TMR_V_ADDR(11);
    scratch_buf_addrs[3] = PHY_TMR_SC_ADDR(VSPA_GO_PHYTIMER_ID);
    scratch_buf_addrs[4] = PHY_TMR_SC_ADDR(VSPA_GO_PHYTIMER_ID);
    scratch_buf_addrs[5] = PHY_TMR_V_ADDR(VSPA_GO_PHYTIMER_ID);

    scratch_buf_values[0] = PHY_TIMER_COMPARATOR_DISABLE | PHY_TIMER_COMPARATOR_CLEAR_INT;
    scratch_buf_values[1] = ePhyTimerComparatorOut0;
    scratch_buf_values[2] = timestamp;
    scratch_buf_values[3] = PHY_TIMER_COMPARATOR_DISABLE | PHY_TIMER_COMPARATOR_CLEAR_INT;
    scratch_buf_values[4] = ePhyTimerComparatorOut1;
    scratch_buf_values[5] = timestamp;

    TRACE_EVENT(T_PHYTIMER, 11, timestamp);
    dmac_enable(DMAC_DI32 | PHY_TMR_DMA_CHAN, 6, VSPA_HALF_WORDS(scratch_buf_addrs), VSPA_HALF_WORDS(scratch_buf_values));
    return PHY_TMR_DMA_CHAN_MASK;
}