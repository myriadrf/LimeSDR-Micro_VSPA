#include "tdd.h"

#include "iohw.h"
#include "dma_common.h"
#include "phytimer.h"
#include "iqstream_signals.h"
#include "vspa.h"

// #if !(PHYTIMERS_THROUGH_M4)

// PHY_TIMER_COMP_RFCTL_4, LMS7002M TXEN pin
// PHY_TIMER_COMP_RFCTL_0, on board RF switch

// Order: DAC DMA allowed (comparator of the selected channel, see tx_tdd_set_dac_allowed_comparator()), PA_EN, RF
// switch. The RF switch comparator (RFCTL_0) is written last so that it can be left out: on LimeSDR-Micro v1+ it is the
// Tx band switch (TX_SW), which LimeSDR_Micro::SetAntenna sets with a forced output. Re-programming it, even with
// "no change", disables it first, and that drops the forced output to 0.
static uint32_t tx_timer_addrs[9] = {
    // DAC DMA
    PHY_TMR_SC_ADDR(PHY_TIMER_COMP_CH5_TX_ALLOWED), PHY_TMR_SC_ADDR(PHY_TIMER_COMP_CH5_TX_ALLOWED),
    PHY_TMR_V_ADDR(PHY_TIMER_COMP_CH5_TX_ALLOWED),
    // PA_EN
    PHY_TMR_SC_ADDR(PHY_TIMER_COMP_RFCTL_5), PHY_TMR_SC_ADDR(PHY_TIMER_COMP_RFCTL_5), PHY_TMR_V_ADDR(PHY_TIMER_COMP_RFCTL_5),
    // RF switch
    PHY_TMR_SC_ADDR(PHY_TIMER_COMP_RFCTL_0), PHY_TMR_SC_ADDR(PHY_TIMER_COMP_RFCTL_0), PHY_TMR_V_ADDR(PHY_TIMER_COMP_RFCTL_0)
};
static uint32_t tx_timer_values[9] = { PHY_TIMER_COMPARATOR_DISABLE | PHY_TIMER_COMPARATOR_CLEAR_INT, // const
                                       0,
                                       0,
                                       PHY_TIMER_COMPARATOR_DISABLE | PHY_TIMER_COMPARATOR_CLEAR_INT, // const
                                       0,
                                       0,
                                       PHY_TIMER_COMPARATOR_DISABLE | PHY_TIMER_COMPARATOR_CLEAR_INT, // const
                                       0,
                                       0 };
// #endif

// The schedule table is shared, which is fine while there is one Tx pipe.
void tx_tdd_set_dac_allowed_comparator(uint16_t phytimer_id) {
    timer_wait_done(); // the table may still be in use by a DMA transfer
    tx_timer_addrs[0] = PHY_TMR_SC_ADDR(phytimer_id);
    tx_timer_addrs[1] = PHY_TMR_SC_ADDR(phytimer_id);
    tx_timer_addrs[2] = PHY_TMR_V_ADDR(phytimer_id);
}

void tx_tdd_schedule_start(const tx_tdd_config_t *tdd, uint32_t phytime) {
    timer_wait_done();
    if (PHYTIMERS_THROUGH_M4) {
        phytimer_data.m4_args.phytime = phytime;
        phytimer_data.m4_args.cmd = M4_PHYTIME_CMD_TDD_START;
        vspa_to_host_signal(1 << 16);
    } else {
        // tx_timer_values[0] = PHY_TIMER_COMPARATOR_DISABLE | PHY_TIMER_COMPARATOR_CLEAR_INT;
        tx_timer_values[1] = ePhyTimerComparatorOut1;
        tx_timer_values[2] = phytime + tdd->dac_allowed;
        // tx_timer_values[3] = PHY_TIMER_COMPARATOR_DISABLE | PHY_TIMER_COMPARATOR_CLEAR_INT;
        tx_timer_values[4] = tdd->pa_on_comparator_out;
        tx_timer_values[5] = phytime + tdd->pa_on;
        // tx_timer_values[6] = PHY_TIMER_COMPARATOR_DISABLE | PHY_TIMER_COMPARATOR_CLEAR_INT;
        tx_timer_values[7] = tdd->rf_sw_on_comparator_out;
        tx_timer_values[8] = phytime + tdd->rf_sw_on;
        const uint16_t count = tdd->rf_sw_on_comparator_out == ePhyTimerComparatorNoChange ? 6 : 9;
        dmac_enable(DMAC_DI32 | PHY_TMR_DMA_CHAN, count, ADDR8(tx_timer_addrs), ADDR8(tx_timer_values));
    }
}

void tx_tdd_schedule_end(const tx_tdd_config_t *tdd, uint32_t phytime, uint32_t rf_switch_phytime) {
    timer_wait_done();
    if (PHYTIMERS_THROUGH_M4) {
        phytimer_data.m4_args.phytime = phytime;
        phytimer_data.m4_args.cmd = M4_PHYTIME_CMD_TDD_END;
        vspa_to_host_signal(1 << 16);
    } else {
        // tx_timer_values[0] = PHY_TIMER_COMPARATOR_DISABLE | PHY_TIMER_COMPARATOR_CLEAR_INT;
        tx_timer_values[1] = ePhyTimerComparatorOut0;
        tx_timer_values[2] = phytime + tdd->dac_allowed;
        // tx_timer_values[3] = PHY_TIMER_COMPARATOR_DISABLE | PHY_TIMER_COMPARATOR_CLEAR_INT;
        tx_timer_values[4] = tdd->pa_off_comparator_out;
        tx_timer_values[5] = phytime + tdd->pa_off;
        // tx_timer_values[6] = PHY_TIMER_COMPARATOR_DISABLE | PHY_TIMER_COMPARATOR_CLEAR_INT;
        tx_timer_values[7] = tdd->rf_sw_off_comparator_out;
        tx_timer_values[8] = rf_switch_phytime + tdd->rf_sw_off;
        const uint16_t count = tdd->rf_sw_off_comparator_out == ePhyTimerComparatorNoChange ? 6 : 9;
        dmac_enable(DMAC_DI32 | PHY_TMR_DMA_CHAN, count, ADDR8(tx_timer_addrs), ADDR8(tx_timer_values));
    }
}
