#include "iqstream_signals.h"

#include "receiver.h"
#include "vspa_iqstream.h"
#include "transmitter.h"

void clear_htv_signal(uint32_t mask) { iowr(HOST_VCPU_FLAGS0, mask); }

void HandleCommandFlags(void) {
    uint32_t flags = iord(HOST_VCPU_FLAGS0);
    for (int lane = 0; lane < RX_MAX_LANE_COUNT; ++lane) {
        if (flags & (HTV_SIGNAL_RXLANE0_ABORT << lane))
            rx_lane_stop(lane);
        if (flags & (HTV_SIGNAL_RXLANE0_PRIME << lane))
            rx_lane_prime(lane);

        if (flags & rx_ddr[lane].ddr.dma.htv_tcd_pending_flag_mask) {
            host_submitted_rx_tcd(lane);
        }
    }
    for (int lane = 0; lane < TX_MAX_LANE_COUNT; ++lane) {
        if (flags & (HTV_SIGNAL_TXLANE0_ABORT << lane))
            tx_lane_abort(lane);
        if (flags & (HTV_SIGNAL_TXLANE0_PRIME << lane))
            tx_lane_prime(lane);

        if (flags & txddr[lane].ddr.dma.htv_tcd_pending_flag_mask) {
            host_submitted_tx_tcd(lane);
        }
    }

    clear_htv_signal(flags); // clear processed flags
}

void vspa_to_host_signal(uint32_t flags) { iowr(VCPU_HOST_FLAGS0, flags); }