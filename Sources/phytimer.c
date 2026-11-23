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
#include "iqstream_signals.h"

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

phytimer_data_t phytimer_data;

void timer_trig_immediate_async(uint16_t id, enum ePhyTimerComparatorTrigger trigger) {
    timer_wait_done();

    if (PHYTIMERS_THROUGH_M4) {
        phytimer_data.m4_args.timer_id = id;
        phytimer_data.m4_args.trig_value = trigger;
        phytimer_data.m4_args.cmd = M4_PHYTIME_CMD_IMMEDIATE;
        vspa_to_host_signal(1 << 16);
    } else {
        phytimer_data.scratch.addrs[0] = PHY_TMR_SC_ADDR(id);
        phytimer_data.scratch.addrs[1] = phytimer_data.scratch.addrs[0];
        phytimer_data.scratch.values[0] = PHY_TIMER_COMPARATOR_DISABLE | PHY_TIMER_COMPARATOR_CLEAR_INT;
        phytimer_data.scratch.values[1] = trigger << 2;

        dmac_enable(DMAC_DI32 | PHY_TMR_DMA_CHAN, 2, ADDR8(phytimer_data.scratch.addrs), ADDR8(phytimer_data.scratch.values));
    }
}

void timer_trig_schedule_async(uint16_t id, enum ePhyTimerComparatorTrigger trigger, uint32_t timestamp) {
    timer_wait_done();
    if (PHYTIMERS_THROUGH_M4) {
        phytimer_data.m4_args.phytime = timestamp;
        phytimer_data.m4_args.timer_id = id;
        phytimer_data.m4_args.trig_value = trigger;
        phytimer_data.m4_args.cmd = M4_PHYTIME_CMD_SCHEDULE;
        vspa_to_host_signal(1 << 16);
    } else {
        phytimer_data.scratch.addrs[0] = PHY_TMR_SC_ADDR(id);
        phytimer_data.scratch.addrs[1] = phytimer_data.scratch.addrs[0];
        phytimer_data.scratch.addrs[2] = PHY_TMR_V_ADDR(id);

        phytimer_data.scratch.values[0] = PHY_TIMER_COMPARATOR_DISABLE | PHY_TIMER_COMPARATOR_CLEAR_INT;
        phytimer_data.scratch.values[1] = trigger;
        phytimer_data.scratch.values[2] = timestamp;
        dmac_enable(DMAC_DI32 | PHY_TMR_DMA_CHAN, 3, ADDR8(phytimer_data.scratch.addrs), ADDR8(phytimer_data.scratch.values));
    }
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
    timer_wait_done();
    // only do Rx triggers, Tx will be scheduled by host
    stream_start_values[5] = timestamp;
    stream_start_values[6] = timestamp;
    stream_start_values[7] = timestamp;
    stream_start_values[8] = timestamp;

    dmac_enable(DMAC_DI32 | PHY_TMR_DMA_CHAN, 9, ADDR8(stream_start_addrs), ADDR8(stream_start_values));
}
