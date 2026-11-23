#ifndef LIME_TIMER_CONTROL_H
#define LIME_TIMER_CONTROL_H

#include <stdbool.h>
#include <stdint.h>

#include "vspa_iqstream.h"
#include "dma_common.h"
#include "iohw.h"
#include "phy-timer.h"

// Use M4 core to configure phytimers by reading arguments from VSPA memory.
// Time wise same cycles duration to completion (cycles count fluctuates depending on bus load in both cases)
// Reduces PRAM usage.
#define PHYTIMERS_THROUGH_M4 0

#define PHY_TMR_DMA_CHAN DDR_WR_DMA_CHANNEL_5
#define PHY_TMR_DMA_CHAN_MASK (1 << PHY_TMR_DMA_CHAN)

#define PHY_TIMER_COMP_VSPA_GO_0 0
#define PHY_TIMER_COMP_CH1_RX_ALLOWED 1
#define PHY_TIMER_COMP_CH2_RX_ALLOWED 2
#define PHY_TIMER_COMP_CH3_RX_ALLOWED 3
#define PHY_TIMER_COMP_CH4_RX_ALLOWED 4
#define PHY_TIMER_COMP_CH5_RX_ALLOWED 5
#define PHY_TIMER_COMP_CH6_RX_ALLOWED 6
#define PHY_TIMER_COMP_CH5_TX_ALLOWED 11
#define PHY_TIMER_COMP_VSPA_GO_1 12
#define PHY_TIMER_COMP_PPS_IN 13
#define PHY_TIMER_COMP_PPS_OUT 14
#define PHY_TIMER_COMP_RFCTL_0 15
#define PHY_TIMER_COMP_RFCTL_1 16
#define PHY_TIMER_COMP_RFCTL_2 17
#define PHY_TIMER_COMP_RFCTL_3 18
#define PHY_TIMER_COMP_RFCTL_4 19
#define PHY_TIMER_COMP_RFCTL_5 20

/* PHY Timer Comparator flags */
/* Clear the interrupt notification bit for this comparator */
#define PHY_TIMER_COMPARATOR_CLEAR_INT (1 << 7)

/* Setting this flag disables this comparator. Enabling a comparator
 * is done by configuring it with a trigger value */
#define PHY_TIMER_COMPARATOR_DISABLE (1 << 6)

#define PHY_TMR_SC_ADDR(id) (PHY_TMR_C0SC + (id * 0x8)) // PHY Timer comparator status and control register address.
#define PHY_TMR_V_ADDR(id) (PHY_TMR_C0V + (id * 0x8)) // PHY Timer comparator status and control register address.

enum ePhyTimerComparatorTrigger {
    ePhyTimerComparatorNoChange = 0x0,
    /** Comparator output signal set to '0' */
    ePhyTimerComparatorOut0,
    /** Comparator output signal set to '1' */
    ePhyTimerComparatorOut1,
    /** Comparator output signal is toggled */
    ePhyTimerComparatorOutToggle,
};

enum M4_PHYTIME_CMD {
    M4_PHYTIME_CMD_IMMEDIATE,
    M4_PHYTIME_CMD_SCHEDULE,
    M4_PHYTIME_CMD_TDD_START,
    M4_PHYTIME_CMD_TDD_END,
};

typedef struct phytimer_ctrl_m4 {
    uint32_t phytime;
    struct {
        uint32_t cmd : 8; // M4_PHYTIME_CMD_*
        uint32_t timer_id : 8;
        uint32_t trig_value : 2;
    };
} m4_phytimer_t;

typedef union {
    m4_phytimer_t m4_args;
    struct {
        uint32_t addrs[3];
        uint32_t values[3];
    } scratch;
} phytimer_data_t;

extern phytimer_data_t phytimer_data;

void phy_tmr_enable(void);
void phy_tmr_configure(void);

void timer_wait_dma_complete(void);
void timer_trig_immediate_async(uint16_t id, enum ePhyTimerComparatorTrigger trigger);
void timer_trig_schedule_async(uint16_t id, enum ePhyTimerComparatorTrigger trigger, uint32_t timestamp);
void stream_trig_schedule_async(uint32_t timestamp);

static inline void timer_wait_done() {
    if (PHYTIMERS_THROUGH_M4) {
        while (iord(VCPU_HOST_FLAGS0) & (1 << 16)) {
        };
    } else {
        WAIT_FOR(!dmac_is_enabled(PHY_TMR_DMA_CHAN_MASK), VSPA_DEFAULT_TIMEOUT);
        // if (dmac_is_enabled(PHY_TMR_DMA_CHAN_MASK))
        //     ++tx_window_stats.phytmr_busy;
    }
}

#endif // LIME_TIMER_CONTROL_H