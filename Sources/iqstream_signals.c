#include "iqstream_signals.h"

#include "receiver.h"
#include "transmitter.h"

void clear_htv_signal(uint32_t mask) { iowr(HOST_VCPU_FLAGS0, mask); }

extern struct DebugStats2 stats2;

void HandleCommandFlags(void) {
    uint32_t flags = iord(HOST_VCPU_FLAGS0);

    for (int lane = 0; lane < RX_MAX_LANE_COUNT; ++lane) {
        if (flags & (HTV_SIGNAL_RXLANE0_ABORT << lane))
            rx_lane_stop(0);
        if (flags & (HTV_SIGNAL_RXLANE0_PRIME << lane))
            rx_lane_prime(0);

        if (flags & rxddr[lane].rx_host_if.htv_pending_flag_mask) {
            flags &= ~(rxddr[lane].rx_host_if.htv_pending_flag_mask);
            if (rx_insert_tcd(lane, &rxddr[lane].rx_host_if.input_tcd))
                clear_htv_signal(rxddr[lane].rx_host_if.htv_pending_flag_mask);
        }
    }
    for (int lane = 0; lane < TX_MAX_LANE_COUNT; ++lane) {
        if (flags & (HTV_SIGNAL_TXLANE0_ABORT << lane))
            tx_lane_abort(0);
        if (flags & (HTV_SIGNAL_TXLANE0_PRIME << lane))
            tx_lane_prime(0);

        if (flags & txddr[lane].dma_hif.htv_pending_flag_mask) {
            flags &= ~(txddr[lane].dma_hif.htv_pending_flag_mask);
            if (tx_insert_tcd(lane, &txddr[lane].dma_hif.input_tcd))
                clear_htv_signal(txddr[lane].dma_hif.htv_pending_flag_mask);
        }
    }

    clear_htv_signal(flags); // clear processed flags
}

void vspa_to_host_signal(uint32_t flags) { iowr(VCPU_HOST_FLAGS0, flags); }