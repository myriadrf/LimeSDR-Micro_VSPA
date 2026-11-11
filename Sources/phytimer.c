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

static uint32_t scratch_buf_addrs[3] = { 0 };
static uint32_t scratch_buf_values[3] = { 0 };

void timer_wait_dma_complete(void) {
    WAIT_FOR(!dmac_is_enabled(PHY_TMR_DMA_CHAN_MASK), VSPA_DEFAULT_TIMEOUT);
    dmac_clear_complete(PHY_TMR_DMA_CHAN_MASK);
}

void timer_trig_immediate_async(uint16_t id, enum ePhyTimerComparatorTrigger trigger) {
    timer_wait_dma_complete();

    scratch_buf_addrs[0] = PHY_TMR_SC_ADDR(id);
    scratch_buf_addrs[1] = scratch_buf_addrs[0];
    scratch_buf_values[0] = PHY_TIMER_COMPARATOR_DISABLE | PHY_TIMER_COMPARATOR_CLEAR_INT;
    scratch_buf_values[1] = trigger << 2;

    dmac_enable(DMAC_DI32 | PHY_TMR_DMA_CHAN, 2, VSPA_HALF_WORDS(scratch_buf_addrs), VSPA_HALF_WORDS(scratch_buf_values));
}

void timer_trig_schedule_async(uint16_t id, enum ePhyTimerComparatorTrigger trigger, uint32_t timestamp) {
    timer_wait_dma_complete();
    scratch_buf_addrs[0] = PHY_TMR_SC_ADDR(id);
    scratch_buf_addrs[1] = scratch_buf_addrs[0];
    scratch_buf_addrs[2] = PHY_TMR_V_ADDR(id);

    scratch_buf_values[0] = PHY_TIMER_COMPARATOR_DISABLE | PHY_TIMER_COMPARATOR_CLEAR_INT;
    scratch_buf_values[1] = trigger;
    scratch_buf_values[2] = timestamp;
    dmac_enable(DMAC_DI32 | PHY_TMR_DMA_CHAN, 3, VSPA_HALF_WORDS(scratch_buf_addrs), VSPA_HALF_WORDS(scratch_buf_values));
}

static const uint32_t stream_start_addrs[9] = { PHY_TMR_SC_ADDR(1), PHY_TMR_SC_ADDR(2),  PHY_TMR_SC_ADDR(3),
                                                PHY_TMR_SC_ADDR(4), PHY_TMR_SC_ADDR(11), PHY_TMR_V_ADDR(1),
                                                PHY_TMR_V_ADDR(2), PHY_TMR_V_ADDR(3),  PHY_TMR_V_ADDR(4) };

static uint32_t stream_start_values[9] = { PHY_TIMER_COMPARATOR_DISABLE | PHY_TIMER_COMPARATOR_CLEAR_INT | ePhyTimerComparatorOut1,
                                           PHY_TIMER_COMPARATOR_DISABLE | PHY_TIMER_COMPARATOR_CLEAR_INT | ePhyTimerComparatorOut1,
                                           PHY_TIMER_COMPARATOR_DISABLE | PHY_TIMER_COMPARATOR_CLEAR_INT | ePhyTimerComparatorOut1,
                                           PHY_TIMER_COMPARATOR_DISABLE | PHY_TIMER_COMPARATOR_CLEAR_INT | ePhyTimerComparatorOut1,
                                           PHY_TIMER_COMPARATOR_DISABLE | PHY_TIMER_COMPARATOR_CLEAR_INT | ePhyTimerComparatorOut1,
                                           0,
                                           0,
                                           0,
                                           0 };

void stream_trig_schedule_async(uint32_t timestamp) {
    timer_wait_dma_complete();
    // only do Rx triggers, Tx will be scheduled by host
    stream_start_values[5] = timestamp;
    stream_start_values[6] = timestamp;
    stream_start_values[7] = timestamp;
    stream_start_values[8] = timestamp;

    dmac_enable(DMAC_DI32 | PHY_TMR_DMA_CHAN, 9, VSPA_HALF_WORDS(stream_start_addrs), VSPA_HALF_WORDS(stream_start_values));
}
