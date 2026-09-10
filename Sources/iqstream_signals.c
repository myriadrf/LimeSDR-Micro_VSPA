#include "iqstream_signals.h"

#include "receiver.h"
#include "transmitter.h"

void clear_htv_signal(uint32_t mask) { iowr(HOST_VCPU_FLAGS0, mask); }

extern struct DebugStats2 stats2;

void HandleCommandFlags(void) {
    uint32_t flags = iord(HOST_VCPU_FLAGS0);

    for (int lane = 0; lane < RX_MAX_LANE_COUNT; ++lane) {
        if (flags & (HTV_SIGNAL_RXLANE0_ABORT << lane))
            rx_lane_stop(lane);
        if (flags & (HTV_SIGNAL_RXLANE0_PRIME << lane))
            rx_lane_prime(lane);

        if (flags & rxddr[lane].dma.htv_tcd_pending_flag_mask) {
            flags &= ~(rxddr[lane].dma.htv_tcd_pending_flag_mask);
            clear_htv_signal(rxddr[lane].dma.htv_tcd_pending_flag_mask);
        }
    }
    for (int lane = 0; lane < TX_MAX_LANE_COUNT; ++lane) {
        if (flags & (HTV_SIGNAL_TXLANE0_ABORT << lane))
            tx_lane_abort(lane);
        if (flags & (HTV_SIGNAL_TXLANE0_PRIME << lane))
            tx_lane_prime(lane);

        if (flags & txddr[lane].dma_hif.htv_tcd_pending_flag_mask) {
            flags &= ~(txddr[lane].dma_hif.htv_tcd_pending_flag_mask);

            tx_lane_try_ddr_enqueue(&txddr[lane]);
            if (!tcd_fifo_isempty(&txddr[lane].dma_hif.tcd_table))
                tx_lane_try_ddr_enqueue(&txddr[lane]);

            clear_htv_signal(txddr[lane].dma_hif.htv_tcd_pending_flag_mask);
        }
    }

    clear_htv_signal(flags); // clear processed flags
}

void vspa_to_host_signal(uint32_t flags) { iowr(VCPU_HOST_FLAGS0, flags); }