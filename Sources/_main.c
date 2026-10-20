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
extern void HandleCommandFlags(void);
extern void proxy_post(void);
extern void rf_iq_comp_params_init(void);

struct VSPA_Stats vspa_core_stats;

bool first_run = true;
inline void HostMBox0Post(uint64_t msg64) { host_mbox0_post(msg64); }

void setup(void) {
    uint64_t msg64 = 0;

    vcpu_swver(0x00020002);

    host_mbox0_post(MAKEDWORD(0xF1000000, 0));
    uint32_t mailbox_in_0_status;
#ifndef IS_SIMULATOR
    do {
        mailbox_in_0_status = (vspa_mbox0_is_valid() | vspa_mbox1_is_valid());
    } while (mailbox_in_0_status == 0);
#endif
    if (mailbox_in_0_status | 0x4) {
        msg64 = host_mbox0_read();
        const uint32_t mailbox_in_msg_0_MSB = HIWORD(msg64);
        const uint32_t mailbox_in_msg_0_LSB = LOWORD(msg64);
        if (mailbox_in_msg_0_MSB == 0x70000000 && mailbox_in_msg_0_LSB == 0x0) {
            host_mbox0_post(MAKEDWORD(0xF0700000, 0x0));
        }
    }

    if (mailbox_in_0_status | 0x8) {
        msg64 = host_mbox1_read();
        const uint32_t mailbox_in_msg_1_MSB = HIWORD(msg64);
        const uint32_t mailbox_in_msg_1_LSB = LOWORD(msg64);
        if (mailbox_in_msg_1_MSB == 0x70000000 && mailbox_in_msg_1_LSB == 0x0) {
            host_mbox1_post(MAKEDWORD(0xF0700000, 0));
        }
    }

    rf_iq_comp_params_init();

    host_clear();
    host_mbox0_enable();
    host_mbox1_enable();
}

__attribute__((noreturn)) static void terminate(void) __noreturn {
    // Control register:
    iowr(CONTROL, 0x00F000A2, 0x0FF01AA3); // disable all GOs
    iowr(DMA_STAT_ABORT, 0xFFFFFFFF);

    // Host interface:
    iowr(HOST_VCPU_FLAGS0, 0xFFFFFFFF);
    iowr(HOST_VCPU_FLAGS1, 0xFFFFFFFF);
    iowr(VCPU_IN_0_MSB, 0xFFFFFFFF);
    iowr(VCPU_IN_1_MSB, 0xFFFFFFFF);
    iowr(VCPU_IN_0_LSB, 0xFFFFFFFF);
    iowr(VCPU_IN_1_LSB, 0xFFFFFFFF);

    // External event interface:
    iowr(EXT_GO_ENA, 0); // Disable all external events.
    iowr(EXT_GO_STAT, 0xFF); // Clear all external events.

    // AXI slave interface:
    // Disable AXI slave events.
    iowr(AXISLV_GOEN1, 0);
    iowr(AXISLV_GOEN0, 0);
    // Clear all AXI slave flags
    iowr(AXISLV_FLAGS0, 0xFFFFFFFF);
    iowr(AXISLV_FLAGS1, 0xFFFFFFFF);

    // DMA engine:
    while (iord(DMA_XRUN_STAT)) {
    }
    iowr(DMA_XFRERR_STAT, 0xFFFFFFFF);
    iowr(DMA_CFGERR_STAT, 0xFFFFFFFF);
    iowr(DMA_COMP_STAT, 0xFFFFFFFF);
    iowr(DMA_GO_STAT, 0xFFFFFFFF);

    // IPPU engine:
    iowr(IPPURC, (0x1 << 29) | (0x1 << 31)); // Abort and clear any pending error.

    // FECU engine:
    iowr(0x300 >> 2, 0x1 << 2); // Disable all pending operations.
    iowr(0x364 >> 2, 0x1 << 10); // Clear error.

    // Entry point & stack pointer:
    iowr(VCPU_GO_ADDR, 0x0); // Initial entry point. PMEM
    // iowr(VCPU_GO_STACK, 0x0); // Stack base address. DMEM
    __builtin_done();
}

__attribute__((noreturn)) void SwReset(void) __noreturn {

    HostMBox0Post(MAKEDWORD(0x0, 0x1));
    axiq_rx_disable();
    axiq_fifo_tx_disable(AXIQ_BANK_0, AXIQ_FIFO_TX0);
    terminate();
}

uint32_t stream_origin_phyticks = 0;

static void stream_start_now(void) {
    const uint32_t phytime_now = gpird(4); // Phytimer
    stream_origin_phyticks = phytime_now + 256;
    stream_trig_schedule_async(ePhyTimerComparatorOut1, stream_origin_phyticks);
    rx_stream_started();
}

extern uint32_t proxy_memory_offset;

uint64_t HandleCommand(uint64_t msg64) {
    const mbox_opc_e op_code = (mbox_opc_e)((HIWORD(msg64) & 0xFF000000) >> 24);
    const uint32_t msg_msb = HIWORD(msg64);
    const uint32_t msg_lsb = LOWORD(msg64);

    const uint32_t fifo_size = (msg_msb & 0x0000FFFF) * 4096;
    const uint32_t fifo_addr = msg_lsb;

    switch (op_code) {
    case MBOX_OPC_RX_CHAN_SELECT: {
        const uint8_t lane = msg_lsb & 0xFF;
        const uint8_t channel = (msg_lsb >> 8) & 0xFF;
        return (MAKEDWORD(0, rx_select_channel(lane, (e_rx_channel)channel)));
        break;
    }
    case MBOX_OPC_RX_CONFIGURE: {
        const uint8_t lane = msg_lsb & 0xFF;
        const uint8_t ovr_pow2 = (msg_lsb >> 8) & 0xFF;
        return (MAKEDWORD(0, rx_set_oversampling(lane, ovr_pow2)));
        break;
    }
    case MBOX_OPC_TX_CONFIGURE: {
        const uint8_t lane = msg_lsb & 0xFF;
        const uint8_t ovr_pow2 = (msg_lsb >> 8) & 0xFF;
        return (MAKEDWORD(0, tx_set_oversampling(lane, ovr_pow2)));
        break;
    }
    case MBOX_OPC_SET_SAMPLE_FORMAT: {
        const uint8_t lane = msg_lsb & 0xFF;
        const iq_sample_format_e format = (iq_sample_format_e)((msg_lsb >> 8) & 0xFF);
        const bool tx = msg_lsb & (1 << 16);
        return (MAKEDWORD(0, tx ? tx_set_sample_format(lane, format) : rx_set_sample_format(lane, format)));
        break;
    }
    case MBOX_OPC_SINGLE_TONE_TX: {
        const uint8_t lane = msg_lsb & 0xFF;
        const bool enable = msg_lsb & (1 << 16);
        return (MAKEDWORD(0, tx_tone_enable(lane, enable)));
        break;
    }
    case MBOX_OPC_IQ_CORR: {
        // QEC param update
        uint32_t iq_channel_id = ((msg_msb & 0x00030000) >> 16); /* bit49-48 */
        uint32_t iq_dir_tx = ((msg_msb & 0x00200000) >> 21); /* bit 53 */
        uint32_t iq_rst = ((msg_msb & 0x00100000) >> 20); /* bit 52 */
        uint32_t iq_idx = msg_msb & 0x0000FFFF; /* bit 47-32*/
        uint32_t iq_val = msg_lsb; /* bit 31-0 */

        if (iq_dir_tx) {
#ifdef TXIQCOMP2
            rf_update_iq_comp_params2(&iq_comp_params2_tx, iq_rst, iq_idx, iq_val);
#else
            rf_update_iq_comp_params(&txiqcompcfg_struct, iq_rst, iq_idx, iq_val);
#endif
        } else {
#ifdef TXIQCOMP2
            rf_update_iq_comp_params2(&iq_comp_params2_rx, iq_rst, iq_idx, iq_val);
#else
            rf_update_iq_comp_params(&rxiqcompcfg_struct, iq_rst, iq_idx, iq_val);
#endif
        }
        return MAKEDWORD(0, 0);
    }
    case MBOX_OPC_DONE_SWRESET:
        SwReset();
        break;

    case MBOX_OPC_GET_FEATURES_MAP: {
        return (MAKEDWORD(VSPA_HALF_WORDS(features_map), lime_Result_Success));
    }
    case MBOX_OPC_RX_FFT: {
        const uint16_t channel = msg_lsb & 0xFF;
        const uint32_t output_address = (uint32_t)rx_fft(channel);
        return (MAKEDWORD(VSPA_HALF_WORDS(output_address), lime_Result_Success));
    }
    case MBOX_OPC_ADC_CAPTURE: {
        const uint16_t channel = msg_lsb & 0xFF;
        const uint32_t output_address = (uint32_t)capture_adc(channel);
        return (MAKEDWORD(VSPA_HALF_WORDS(output_address), lime_Result_Success));
    }
    case MBOX_OPC_STREAM_START: {
        stream_start_now();
        return (MAKEDWORD(0, lime_Result_Success));
    }
    case MBOX_OPC_TRACE_RESET: {
        l1_trace_clear();
        TRACE_EVENT(T_TRACE_PUSH, DEFAULT_THREAD_ID, 0);
        return (MAKEDWORD(0, lime_Result_Success));
    }
    case MBOX_OPC_PROXY_OFFSET: {
        return (MAKEDWORD(0, proxy_set_address(msg_lsb)));
    }

    default:
        // not a valid command, NACK
        return (MAKEDWORD(op_code, lime_Result_InvalidValue));
        break;
    }
    return lime_Result_Error;
}

// called by bootloader on first run
static void BootEntry(void) {
    l1_trace_init();

    // !! this workaround is needed otherwise VSPA gets stuck ( dmac_is_available()/dmac_is_complete() doesn't work)
    // !! unless issue "ccs::config_chain {la9310 dap}" on ccs
    const uint16_t HOST_VCPU_A011455 = (0x024 >> 2);
    iowr(HOST_VCPU_A011455, 0x10, 0x10);
    setup();

    // iowr(IRQEN, 0x10, 0x10); // irqen_dma_cmp

    // clear HOST_GO bit
    iowr(CONTROL,
         0x0 | (1 << 25) // host msg1 go
             | (1 << 24) // host msg0 go
             | (1 << 11) // host to vcpu flags0 go
         ,
         0x0F000001 | (1 << 11));

    iowr(EXT_GO_ENA, 0xFF, 0xFF); // enable exteral GO event
    // entry(main);
}

inline void ProcessMBox(void) {
    if (vspa_mbox0_is_valid()) // message from Host
    {
        const uint64_t cmd = host_mbox0_read();
        host_mbox0_clear();
        host_mbox0_post(HandleCommand(cmd));
    }
    if (vspa_mbox1_is_valid()) // message from Arm M4
    {
        const uint64_t cmd = host_mbox1_read();
        host_mbox1_clear();
        host_mbox1_post(HandleCommand(cmd));
    }
}

#define GO_REASON_HOST_READ_MSG1 (1 << 23)
#define GO_REASON_HOST_READ_MSG0 (1 << 22)
#define GO_REASON_HOST_SENT_MSG1 (1 << 21)
#define GO_REASON_HOST_SENT_MSG0 (1 << 20)
#define GO_REASON_AXI (1 << 8)
#define GO_REASON_FECU (1 << 7)
#define GO_REASON_HOST_VSP_FLAGS (1 << 6)
#define GO_REASON_DEBUG_MSG (1 << 5)
#define GO_REASON_VCPU (1 << 4)
#define GO_REASON_EXT (1 << 3)
#define GO_REASON_DMA (1 << 2)
#define GO_REASON_IPPU (1 << 1)
#define GO_REASON_HOST (1 << 0)

void ProcessTxDMA(void) {
    if (dmac_is_complete(1 << 7) || dmac_event(1 << 7))
        tx_ddr_complete(0);
    tx_check_axiq_status();
    if (dmac_event(1 << 11))
        dac_dma_complete(0);
}

// gets called by event triggers
__attribute__((noreturn)) void main(void) {
    const uint32_t t1 = ccnt_read();
    if (first_run) {
        BootEntry();
        entry(main);
        first_run = false;

        receiver_init();
        transmitter_init();
    }
    // TRACE_START_DURATION(t1);
    ++vspa_core_stats.go_count;
    const uint32_t ctrl = iord(CONTROL);
    TRACE_COUNTER(CNT_TX_DMA_ALLOWED, gpird(1, 1 << 16));

    // DMA is time critical, process it first
    if (ctrl & GO_REASON_EXT) // phytimer triggered GO
    {
        TRACE_EVENT(T_EXTERNAL_GO, 1, 0);
        ++vspa_core_stats.ext_go_count;
        tx_external_trigger();

        // iowr(EXT_GO_STAT, 0xFF, 0xFF); // clear external GO
    }

    // if (ctrl & GO_REASON_DMA)
    {
        // could check status only on completion, but completion would not get triggered
        // if underrun cause DMA xfer extend past phytimer disable point and get stuck.
        if (dmac_is_complete(1 << 13))
            rx_ddr_dma_complete(1);
        if (dmac_is_complete(1 << 12))
            rx_ddr_dma_complete(0);
        ProcessTxDMA();

        for (int lane = 0; lane < RX_MAX_LANE_COUNT; ++lane) {
            if (dmac_event(1 << rx_adc[lane].dma_channel))
                adc_dma_complete(lane);
        }
        // if (compl & (1<<0))
        //     dma_done_callback[0]();
    }

    if (ctrl & (GO_REASON_HOST_SENT_MSG0 | GO_REASON_HOST_SENT_MSG1)) {
        TRACE_START_DURATION(t2);
        ProcessMBox();
        TRACE_DURATION(T_MBOX, 1, t2);
    }

    if (ctrl & GO_REASON_HOST_VSP_FLAGS) {
        HandleCommandFlags();
    }

    if (proxy_dirty)
        proxy_post();

    push_traces();

    TRACE_DURATION(T_BUSY, DEFAULT_THREAD_ID, t1);

    // vspa_core_stats.busy_cycles += ccnt_read() - cyccnt;
    __builtin_done();
}
