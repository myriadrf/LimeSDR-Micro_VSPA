#ifndef LIME_TIMER_CONTROL_H
#define LIME_TIMER_CONTROL_H

#include <stdbool.h>
#include <stdint.h>

#define VSPA_GO_PHYTIMER_ID 0

enum ePhyTimerComparatorTrigger {
    ePhyTimerComparatorNoChange = 0x0,
    /** Comparator output signal set to '0' */
    ePhyTimerComparatorOut0,
    /** Comparator output signal set to '1' */
    ePhyTimerComparatorOut1,
    /** Comparator output signal is toggled */
    ePhyTimerComparatorOutToggle,
};

void phy_tmr_enable(void);
void phy_tmr_configure(void);

uint16_t timer_trig_immediate(uint32_t id, enum ePhyTimerComparatorTrigger trigger);
uint32_t timer_trig_immediate_async(uint32_t id, enum ePhyTimerComparatorTrigger trigger);
uint16_t timer_trig_schedule(uint32_t id, enum ePhyTimerComparatorTrigger trigger, uint32_t timestamp);
uint16_t timer_trig_schedule_async(uint32_t id, enum ePhyTimerComparatorTrigger trigger, uint32_t timestamp);

void check_timer_dma(void);

#endif // LIME_TIMER_CONTROL_H