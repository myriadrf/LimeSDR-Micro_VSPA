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
#include "vspa_state.h"
#include "vspa_features.h"

#include "dma_common.h"
#include "platform.h"
#include "timer_control.h"

#include "compiler.h"

extern const feature_t features_map[];

D_STATIC bool first_run = true;
D_STATIC inline void HostMBox0Post(uint64_t msg64) { host_mbox0_post(msg64); }

D_STATIC void setup(void) {
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

    memclr(&player_state, sizeof(player_state));
}

D_STATIC __attribute__((noreturn)) void SwReset(void) __noreturn {
    extern void start(void);
    entry(start);
    host_clear();
    dmac_clear_event();
    HostMBox0Post(MAKEDWORD(0x0, 0x1));
    __idle();
}

D_STATIC uint64_t HandleCommand(uint64_t msg64) {
    const mbox_opc_e op_code = (mbox_opc_e)((HIWORD(msg64) & 0xFF000000) >> 24);
    const uint32_t msg_msb = HIWORD(msg64);
    const uint32_t msg_lsb = LOWORD(msg64);

    const e_rx_channel rx_index = (e_rx_channel)((msg_msb & 0x00300000) >> 20);
    const uint32_t fifo_size = (msg_msb & 0x0000FFFF) * 4096;
    const uint32_t fifo_addr = msg_lsb;

    switch (op_code) {
    case MBOX_OPC_RX_HOST_FIFO_CONFIG: {
        ConfigRxHostFIFO(rx_index, fifo_addr, fifo_size);
        return (MAKEDWORD(op_code, lime_Result_Success));
        break;
    }
    case MBOX_OPC_TX_CONFIGURE: {
        const uint32_t interpolation = 1 << (msg_lsb & 0x3);
        return (MAKEDWORD(op_code, TxConfigure(interpolation)));
        break;
    }
    case MBOX_OPC_RX_CONFIGURE: {
        const uint32_t decimation = 1 << (msg_lsb & 0x3);
        return (MAKEDWORD(op_code, RxChannelConfigure(rx_index, decimation)));
        break;
    }
    case MBOX_OPC_TX_CONTROL:
        return (MAKEDWORD(op_code, TxDDR_control(msg64)));
        break;
    case MBOX_OPC_RX_CONTROL:
        return (MAKEDWORD(op_code, RxDDR_control(rx_index, msg64)));
        break;

    case MBOX_OPC_SINGLE_TONE_TX:
        TxTone_control(msg64);
        return (MAKEDWORD(0, 0x1));
        break;

    case MBOX_OPC_SINGLE_TONE_RX:
        // RX_single_tone_measurement();
        break;

    case MBOX_OPC_RX_CHAN_SELECT:
        return (MAKEDWORD(op_code, RxChannelSelect(msg_lsb & 0xf)));
        break;

    case MBOX_OPC_DONE_SWRESET:
        SwReset();
        break;

    case MBOX_OPC_PROXY_OFFSET: {
        const uint32_t proxy_offset_read_only = ((msg_msb & 0x00100000) >> 20); /* bit 52 */
        if (!proxy_offset_read_only)
            dmem_proxy_set_offset(msg_lsb);
        return (MAKEDWORD(msg_lsb, lime_Result_Success));
        break;
    }

    case MBOX_OPC_RX_PREPARE: {
        return (MAKEDWORD(op_code, RxPrepare()));
    }

    case MBOX_OPC_GET_FEATURES_MAP: {
        return (MAKEDWORD(VSPA_HALF_WORDS(features_map), lime_Result_Success));
    }

    case MBOX_OPC_TX_DMA_SUBMIT: {
        return (MAKEDWORD(op_code, TxDMASubmit()));
    }

    default:
        // not a valid command, NACK
        return (MAKEDWORD(op_code, lime_Result_InvalidValue));
        break;
    }
    return 0;
}

// called by bootloader on first run
void BootEntry(void) {
    dmac_clear_error();
    dmac_clear_complete();
    dmac_clear_event();
    l1_trace_init();

    // !! this workaround is needed otherwise VSPA gets stuck ( dmac_is_available()/dmac_is_complete() doesn't work)
    // !! unless issue "ccs::config_chain {la9310 dap}" on ccs
    const uint16_t HOST_VCPU_A011455 = (0x024 >> 2);
    iowr(HOST_VCPU_A011455, 0x10, 0x10);
    setup();

    // update host vspa_dmem_proxy
    player_state.info.dmemProxyOffset = (uint32_t)&player_state;

    InitializeRx();
    InitializeTx();

    iowr(IRQEN, 0x10, 0x10); // irqen_dma_cmp

    // clear HOST_GO bit
    iowr(CONTROL,
         0x0 | (1 << 25) // host msg1 go
             | (1 << 24) // host msg0 go
             | (1 << 11) // host to vcpu flags go
         ,
         0x0F000001 | (1 << 11));

    iowr(EXT_GO_ENA, 0x1, 0xFF); // enable exteral GO event
    // entry(main);
}

D_STATIC inline void ProcessMBox(void) {
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

D_STATIC inline void ProcessHostFlags(void) {
    const uint32_t flags = iord(HOST_VCPU_FLAGS0);
    switch (flags) {
    case 0:
        return;
    case 1:
        HostProducedEvent();
        break;
    }
    iowr(HOST_VCPU_FLAGS0, 0xFFFFFFFF);
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

// void EmptyFunc(void)
// {}

// typedef void (*void_fptr)(void);
// static const void_fptr dma_done_callback[16] =
// {
//     &EmptyFunc, // DDR_WR_DMA_CHANNEL_5
//     &OnADCRead_Completed, // RO0_ADC_RD_DMA_CHANNEL
//     &OnADCRead_Completed, // RO1_ADC_RD_DMA_CHANNEL
//     &OnADCRead_Completed, // RX0_ADC_RD_DMA_CHANNEL
//     &OnADCRead_Completed, // RX1_ADC_RD_DMA_CHANNEL
//     &EmptyFunc, // AUX_ADC
//     &EmptyFunc, // RSSI
//     &OnDDRRD_Completed, // DDR_RD_1
//     &EmptyFunc, // DDR_RD_2
//     &EmptyFunc, // DDR_RD_3
//     &EmptyFunc, // DDR_RD_4
//     &OnDACWrite_Completed, // TX_DAC
//     &EmptyFunc, // DDR_WR_1
//     &check_l1_trace_complete, // DDR_WR_2
//     &OnDDRWR_Completed, // DDR_WR_3
//     &VSPA_PROXY_complete, // DDR_WR_4
// };

// gets called by event triggers
__attribute__((noreturn)) void main(void) {
    // TRACE_START_DURATION(t1);
    if (first_run) {
        BootEntry();
        entry(main);
        first_run = false;
        iowr(EXT_GO_STAT, 0xFF, 0xFF); // clear external GO
        timer_trig_immediate(VSPA_GO_PHYTIMER_ID, ePhyTimerComparatorOut0); // GO triggered only by rising edge
    }
    const uint32_t ctrl = iord(CONTROL);
    TRACE_BEGIN(T_GO, 1, ctrl);
    ++player_state.internals.go_count;

    // DMA is time critical, process it first
    if (ctrl & GO_REASON_EXT) // phytimer triggered GO
    {
        TRACE_EVENT(T_EXTERNAL_GO, 1, 0);

        if (DAC_Flush()) {
            iowr(EXT_GO_STAT, 0xFF, 0xFF); // clear external GO
        }
    }

    // if (ctrl & GO_REASON_DMA)
    uint32_t compl = dmac_is_complete(0xFFFF);
    if (compl ) {
        // const uint32_t events = dmac_event();
        // TRACE_EVENT(TG_VCPU, T_GO, compl, 1);

        // dmac_clear_event(events);
        if (compl &(1 << 15))
            VSPA_PROXY_complete(); // dma_done_callback[15]();
        // if (compl & (1<<14))
        //     OnDDRWR_Completed();//dma_done_callback[14]();
        if (dmac_event(1 << 13))
            OnDDRWR_Completed(1);
        if (dmac_event(1 << 12))
            OnDDRWR_Completed(0);
        if (dmac_event(1 << 11))
            OnDACWrite_Completed(); // dma_done_callback[11]();
        // if (compl & (1<<10))
        //     dma_done_callback[10]();
        // if (compl & (1<<9))
        //     dma_done_callback[9]();
        // if (compl & (1<<8))
        //     dma_done_callback[8]();
        if (dmac_event(1 << 7))
            OnDDRRD_Completed(); // dma_done_callback[7]();
        // if (compl & (1<<6))
        //     dma_done_callback[6]();
        // if (compl & (1<<5))
        //     dma_done_callback[5]();
        if (dmac_event(1 << 4)) // RX1
            OnADCRead_Completed(VSPA_RX1);
        if (dmac_event(1 << 3)) // RX0
            OnADCRead_Completed(VSPA_RX0);
        if (dmac_event(1 << 2))
            OnADCRead_Completed(VSPA_RO1);
        if (dmac_event(1 << 1))
            OnADCRead_Completed(VSPA_RO0);
        // if (compl & (1<<0))
        //     dma_done_callback[0]();
    }

    // TRACE_DURATION(T_GO, 1, t1);

    if (ctrl & (GO_REASON_HOST_SENT_MSG0 | GO_REASON_HOST_SENT_MSG1)) {
        TRACE_START_DURATION(t2);
        ProcessMBox();
        TRACE_DURATION(T_MBOX, 1, t2);
    }

    if (ctrl & GO_REASON_HOST_VSP_FLAGS) {
        // TRACE_START_DURATION(t3);
        ProcessHostFlags();
        // TRACE_DURATION(T_HOST_PRODUCE, 1, t3);
    }

    VSPA_PROXY_update();
    push_traces();

    TRACE_END(T_GO, 1, ctrl);
    __builtin_done();
}
