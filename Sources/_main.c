/* SPDX-License-Identifier: BSD-3-Clause */

/*
 * Copyright 2024 NXP
 */

#include "iqplayer_commands.h"

#include "host.h"
#include "dmac.h"
#include "dfe.h"
#include "transmitter.h"
#include "receiver.h"
#include "l1-trace.h"
#include "vspa_memorymap.h"

#include "dma_common.h"
#include "axiq-la9310.h"

#include "opstatus.h"
#include "iqstream_signals.h"
#include "vspa_iqstream.h"
#include "phytimer.h"
#include "proxy_data.h"

extern const vspa_feature_t features_map[];
extern void proxy_post(void);

struct VSPA_Stats vspa_core_stats;

static inline void MBoxPost(int16_t mbox, uint64_t msg64) {
    // if (mbox == 0)
    //     host_mbox0_post(msg64);
    // else
    //     host_mbox1_post(msg64);
    iowr64(VCPU_OUT_0_MSB + mbox * 8, msg64);
}

static void HandleCommandFlags(void) {
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

static void clean_state(void) {
    // enter DMA flush
    axiq_rx_disable();
    axiq_fifo_tx_disable(AXIQ_BANK_0, AXIQ_FIFO_TX0);

    // abort all DMA operations
    iowr(DMA_STAT_ABORT, 0xFFFFFFFF);
    iowr(IPPURC, (0x1 << 29) | (0x1 << 31)); // Abort IPPU and clear any pending error.
    iowr(0x300 >> 2, 0x1 << 2); // FECU disable all pending operations.
    iowr(0x364 >> 2, 0x1 << 10); // FECU clear errors

    // disable GO event sources
    iowr(EXT_GO_ENA, 0); // Disable all external events.
    iowr(EXT_GO_STAT, 0xFF); // Clear all external events, GOs are triggred by rising edges
    iowr(CONTROL, 0x0FF01AA3, 0x00F000A2);

    // Host interface:
    iowr(HOST_VCPU_FLAGS0, 0xFFFFFFFF);
    iowr(HOST_VCPU_FLAGS1, 0xFFFFFFFF);

    // readout host to VCPU messages
    iord(VCPU_IN_0_LSB);
    iord(VCPU_IN_1_LSB);

    // AXI slave interface:
    // Disable AXI slave events.
    // iowr(AXISLV_GOEN1, 0);
    // iowr(AXISLV_GOEN0, 0);
    // Clear all AXI slave flags
    // iowr(AXISLV_FLAGS0, 0xFFFFFFFF);
    // iowr(AXISLV_FLAGS1, 0xFFFFFFFF);

    // DMA engine:
    // probably should add timeout in case DMA engine would be stuck
    while (iord(DMA_XRUN_STAT)) {
    }

    // DMA engine state
    iowr(DMA_XFRERR_STAT, 0xFFFFFFFF);
    iowr(DMA_CFGERR_STAT, 0xFFFFFFFF);
    iowr(DMA_COMP_STAT, 0xFFFFFFFF);
    iowr(DMA_GO_STAT, 0xFFFFFFFF);

    // According to manual, CONTROL host_go bit when set, it requests that VCPU
    // start processing instructions at PMEM address 0 (assuming VCPU is stopped).
    // That seems to be wrong, it does not force start at PMEM 0x0, and just starts
    // execution at VCPU_GO_ADDR, so must set it to 0 for next firmware load.
    vcpu_swver(0); // firmware is still available in memory, but treat 0 version as abandoned
    entry(0);
}

static inline __attribute__((noreturn)) void SwReset(void) __noreturn {
    host_mbox0_post(MAKEDWORD(0x0, 0x1));
    clean_state();
    __fnop();
    __fnop();
    __builtin_done();
}

uint32_t stream_origin_phyticks = 0;

static inline void stream_start_now(uint32_t phytime, bool specific_time) {
    if (specific_time) {
        stream_origin_phyticks = phytime;
    } else {
        const uint32_t phytime_now = gpird(4); // Phytimer
        stream_origin_phyticks = phytime_now + 256;
    }
    stream_trig_schedule_async(stream_origin_phyticks);
    rx_stream_started();
}

void HandleCommand(uint64_t msg64, int16_t mbox) {
    const mbox_opc_e op_code = (mbox_opc_e)((HIWORD(msg64) & 0xFF000000) >> 24);
    const uint32_t msg_msb = HIWORD(msg64);
    const uint32_t msg_lsb = LOWORD(msg64);

    switch (op_code) {
    case MBOX_OPC_RX_CHAN_SELECT: {
        const uint8_t lane = msg_lsb & 0xFF;
        const uint8_t channel = (msg_lsb >> 8) & 0xFF;
        msg64 = (MAKEDWORD(0, rx_select_channel(lane, (e_rx_channel)channel)));
        break;
    }
    case MBOX_OPC_TX_CHAN_SELECT: {
        const uint8_t pipe = msg_lsb & 0xFF;
        const uint8_t channel = (msg_lsb >> 8) & 0xFF;
        msg64 = (MAKEDWORD(0, tx_select_channel(pipe, (e_tx_channel)channel)));
        break;
    }
    case MBOX_OPC_SET_SAMPLE_FORMAT: {
        const uint8_t lane = msg_lsb & 0xFF;
        const iq_sample_format_e format = (iq_sample_format_e)((msg_lsb >> 8) & 0xFF);
        const bool tx = msg_lsb & (1 << 16);
        msg64 = (MAKEDWORD(0, tx ? tx_set_sample_format(lane, format) : rx_set_sample_format(lane, format)));
        break;
    }
    case MBOX_OPC_SINGLE_TONE_TX: {
        const uint8_t lane = msg_lsb & 0xFF;
        const bool enable = msg_lsb & (1 << 16);
        msg64 = (MAKEDWORD(0, tx_tone_enable(lane, enable)));
        break;
    }
    case MBOX_OPC_DONE_SWRESET:
        SwReset();
        break;

    case MBOX_OPC_GET_FEATURES_MAP: {
        msg64 = (MAKEDWORD(ADDR8(features_map), lime_Result_Success));
        break;
    }
    case MBOX_OPC_RX_FFT: {
        const uint32_t freq_ctrl_word = msg_lsb;
        const uint16_t channel = msg_msb & 0x3;
        const int status = rx_dft_start((e_rx_channel)channel, freq_ctrl_word);
        if (status != lime_Result_Success)
            msg64 = (MAKEDWORD(0, (uint32_t)status));
        return;
    }
    case MBOX_OPC_STREAM_START: {
        const uint32_t phytime = msg_lsb;
        const bool specific_time = msg_msb & 0x1;
        stream_start_now(phytime, specific_time);
        msg64 = (MAKEDWORD(0, lime_Result_Success));
        break;
    }
    case MBOX_OPC_TRACE_RESET: {
        l1_trace_clear();
        TRACE_EVENT(T_TRACE_PUSH, DEFAULT_THREAD_ID, 0);
        msg64 = (MAKEDWORD(0, lime_Result_Success));
        break;
    }
    case MBOX_OPC_PROXY_OFFSET: {
        msg64 = (MAKEDWORD(0, proxy_set_address(msg_lsb)));
        break;
    }

    default:
        // not a valid command, NACK
        msg64 = (MAKEDWORD(op_code, lime_Result_InvalidValue));
        break;
    }
    MBoxPost(mbox, msg64);
}

static inline void ProcessMBox(void) {
    if (vspa_mbox0_is_valid()) // message from Host
    {
        HandleCommand(host_mbox0_read(), 0);
    }
    if (vspa_mbox1_is_valid()) // message from Arm M4
    {
        HandleCommand(host_mbox1_read(), 1);
    }
}

#define CONTROL_GO_REASON_HOST_READ_MSG1 (1 << 23)
#define CONTROL_GO_REASON_HOST_READ_MSG0 (1 << 22)
#define CONTROL_GO_REASON_HOST_SENT_MSG1 (1 << 21)
#define CONTROL_GO_REASON_HOST_SENT_MSG0 (1 << 20)
#define CONTROL_GO_REASON_AXI (1 << 8)
#define CONTROL_GO_REASON_FECU (1 << 7)
#define CONTROL_GO_REASON_HOST_VSP_FLAGS (1 << 6)
#define CONTROL_GO_REASON_DEBUG_MSG (1 << 5)
#define CONTROL_GO_REASON_VCPU (1 << 4)
#define CONTROL_GO_REASON_EXT (1 << 3)
#define CONTROL_GO_REASON_DMA (1 << 2)
#define CONTROL_GO_REASON_IPPU (1 << 1)
#define CONTROL_GO_REASON_HOST (1 << 0)

void ProcessTxDMA(void) {
    if (dmac_is_complete(1 << 7))
        tx_ddr_complete(0);
    tx_check_axiq_status();
    if (dmac_event(1 << 11))
        dac_dma_complete(0);
}

// gets called by event triggers
__attribute__((noreturn)) void main_go(void) {
    const uint32_t t1 = ccnt_read();
    ++vspa_core_stats.go_count;
    const uint32_t ctrl = iord(CONTROL);
    TRACE_COUNTER(CNT_TX_DMA_ALLOWED, gpird(1, 1 << 16));

    // DMA is time critical, process it first
    if (ctrl & CONTROL_GO_REASON_EXT) // phytimer triggered GO
    {
        TRACE_EVENT(T_EXTERNAL_GO, 1, 0);
        ++vspa_core_stats.ext_go_count;
        tx_external_trigger();
        // iowr(EXT_GO_STAT, 0xFF, 0xFF); // clear external GO
    }

    // if (ctrl & CONTROL_GO_REASON_DMA)
    {
        if (dmac_is_complete(1 << 13))
            rx_ddr_dma_complete(1);
        if (dmac_is_complete(1 << 12))
            rx_ddr_dma_complete(0);
        ProcessTxDMA();

        for (int lane = 0; lane < RX_PIPE_COUNT; ++lane) {
            if (dmac_event(1 << afe_dma_channel(&rx_adc[lane].afe)))
                rx_pipe_process(lane);
        }
    }

    if (ctrl & (CONTROL_GO_REASON_HOST_SENT_MSG0 | CONTROL_GO_REASON_HOST_SENT_MSG1)) {
        TRACE_START_DURATION(t2);
        ProcessMBox();
        TRACE_DURATION(T_MBOX, 1, t2);
    }

    if (ctrl & CONTROL_GO_REASON_HOST_VSP_FLAGS) {
        HandleCommandFlags();
    }

    if (proxy_dirty)
        proxy_post();

    push_traces();

    TRACE_DURATION(T_BUSY, DEFAULT_THREAD_ID, t1);

    host_clear(); // just in case of rogue host_go, TODO: can clear together with mailbox status clearing
    vspa_core_stats.busy_cycles += ccnt_read() - t1;
    __builtin_done();
}

// Program entry from crt0 on every start at PMEM 0:
// Do first time configuration, then every following GO event enters main_go().
// After SwReset() a stray GO also starts here, which just restarts the firmware.
__attribute__((noreturn)) void main(void) {
    // !! this workaround is needed otherwise VSPA gets stuck ( dmac_is_available()/dmac_is_complete() doesn't work)
    // !! unless issue "ccs::config_chain {la9310 dap}" on ccs
    const uint16_t HOST_VCPU_A011455 = (0x024 >> 2);
    iowr(HOST_VCPU_A011455, 0x10, 0x10);

    clean_state();

    l1_trace_init();
    qec_init();

    receiver_init();
    transmitter_init();

    iowr(EXT_GO_ENA, 0xFF, 0xFF); // enable exteral GO event

    const uint32_t irqen_bits = (1 << 4) | (1 << 2); // irqen_dma_cmp, irqen_flags0
    iowr(IRQEN, irqen_bits, irqen_bits);

    entry(main_go);
    vcpu_swver(0x00020003); // hint for host that valid firmware is present

    // must clear host_go, only VCPU can clear it. If not cleared VSPA will be stuck in "bootloop".
    // Clearing of host_go indicates to host that VSPA has completed bootup.
    iowr(CONTROL, 0x0F000001 | (1 << 11),
         0x0 | (1 << 25) // host msg1 go
             | (1 << 24) // host msg0 go
             | (1 << 11) // host to vcpu flags0 go
             | (1 << 0) // clear host_go
    );
    __fnop();
    __fnop();
    __builtin_done();
}