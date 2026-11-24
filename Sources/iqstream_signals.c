#include "iqstream_signals.h"

#include "receiver.h"
#include "vspa_iqstream.h"
#include "transmitter.h"

void HandleCommandFlags(void) {
    uint32_t flags = iord(HOST_VCPU_FLAGS0);
    for (int lane = 0; lane < RX_PIPE_COUNT; ++lane) {
        if (flags & (HTV_SIGNAL_RXLANE0_ABORT << lane))
            rx_lane_stop(lane);
        if (flags & (HTV_SIGNAL_RXLANE0_PRIME << lane))
            rx_lane_prime(lane);

        if (flags & rx_hdma[lane].htv_tcd_pending_flag_mask) {
            host_submitted_rx_tcd(lane);
        }
    }
    for (int lane = 0; lane < TX_PIPE_COUNT; ++lane) {
        if (flags & (HTV_SIGNAL_TXLANE0_ABORT << lane))
            tx_lane_abort(lane);
        if (flags & (HTV_SIGNAL_TXLANE0_PRIME << lane))
            tx_lane_prime(lane);

        if (flags & tx_hdma[lane].htv_tcd_pending_flag_mask) {
            host_submitted_tx_tcd(lane);
        }
    }

    clear_htv_signal(flags); // clear processed flags
}
