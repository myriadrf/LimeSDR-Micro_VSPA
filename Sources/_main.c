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

vspa_state_t state;

extern const vspa_feature_t features_map[];

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
    axiq_fifo_rx_disable(AXIQ_BANK_0, AXIQ_FIFO_RX0);
    axiq_fifo_rx_disable(AXIQ_BANK_0, AXIQ_FIFO_RX1);
    axiq_fifo_rx_disable(AXIQ_BANK_0, AXIQ_FIFO_RX2);
    axiq_fifo_rx_disable(AXIQ_BANK_0, AXIQ_FIFO_RX3);
    axiq_fifo_tx_disable(AXIQ_BANK_0, AXIQ_FIFO_TX0);
    terminate();
}

uint64_t HandleCommand(uint64_t msg64) {
    const mbox_opc_e op_code = (mbox_opc_e)((HIWORD(msg64) & 0xFF000000) >> 24);
    const uint32_t msg_msb = HIWORD(msg64);
    const uint32_t msg_lsb = LOWORD(msg64);

    const uint32_t fifo_size = (msg_msb & 0x0000FFFF) * 4096;
    const uint32_t fifo_addr = msg_lsb;

    switch (op_code) {
    case MBOX_OPC_SINGLE_TONE_TX:
        // TxTone_control(msg64);
        return (MAKEDWORD(0, 0x1));
        break;

    case MBOX_OPC_DONE_SWRESET:
        SwReset();
        break;

    case MBOX_OPC_GET_FEATURES_MAP: {
        return (MAKEDWORD(VSPA_HALF_WORDS(features_map), lime_Result_Success));
    }

    default:
        // not a valid command, NACK
        return (MAKEDWORD(op_code, lime_Result_InvalidValue));
        break;
    }
    return 0;
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

    iowr(EXT_GO_ENA, 0x1, 0xFF); // enable exteral GO event
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

// gets called by event triggers
__attribute__((noreturn)) void main(void) {
    if (first_run) {
        BootEntry();
        entry(main);
        first_run = false;

        receiver_init();
        transmitter_init();
    }
    ++state.go_count;

    TRACE_START_DURATION(t1);
    const uint32_t ctrl = iord(CONTROL);

    // TRACE_BEGIN(T_GO, 1, ctrl);

    // DMA is time critical, process it first
    if (ctrl & GO_REASON_EXT) // phytimer triggered GO
    {
        TRACE_EVENT(T_EXTERNAL_GO, 1, 0);

        // iowr(EXT_GO_STAT, 0xFF, 0xFF); // clear external GO
    }

    if (ctrl & GO_REASON_DMA) {
        // const uint32_t events = dmac_event();
        // TRACE_EVENT(TG_VCPU, T_GO, compl, 1);

        // dmac_clear_event(events);
        // if (compl &(1 << 15))
        //     VSPA_PROXY_complete(); // dma_done_callback[15]();
        // if (compl & (1<<14))
        //     OnDDRWR_Completed();//dma_done_callback[14]();
        if (dmac_is_complete(1 << 13))
            ddr_dma_complete(1);
        if (dmac_is_complete(1 << 12))
            ddr_dma_complete(0);
        if (dmac_event(1 << 11))
            dac_dma_complete(0);
        // if (compl & (1<<10))
        //     dma_done_callback[10]();
        // if (compl & (1<<9))
        //     dma_done_callback[9]();
        // if (compl & (1<<8))
        //     dma_done_callback[8]();
        if (dmac_event(1 << 7))
            tx_ddr_complete(0); // dma_done_callback[7]();
        // if (compl & (1<<6))
        //     dma_done_callback[6]();
        // if (compl & (1<<5))
        //     dma_done_callback[5]();
        if (dmac_event(1 << 4)) // RX1
            adc_dma_complete(1);
        if (dmac_event(1 << 3)) // RX0
            adc_dma_complete(0);
        if (dmac_event(1 << 2))
            adc_dma_complete(1);
        if (dmac_event(1 << 1))
            adc_dma_complete(0);
        // if (compl & (1<<0))
        //     dma_done_callback[0]();
    }

    if (ctrl & (GO_REASON_HOST_SENT_MSG0 | GO_REASON_HOST_SENT_MSG1)) {
        TRACE_START_DURATION(t2);
        ProcessMBox();
        TRACE_DURATION(T_MBOX, 1, t2);
    }

    if (ctrl & GO_REASON_HOST_VSP_FLAGS) {
        const uint32_t f = iord(HOST_VCPU_FLAGS0);
        TRACE_START_DURATION(t3);
        HandleCommandFlags();
        TRACE_DURATION(T_HOST_PRODUCE, f, t3);
    }

    TRACE_DURATION(T_GO, ctrl, t1);
    // TRACE_END(T_GO, 1, ctrl);
    // VSPA_PROXY_update();
    push_traces();

    __builtin_done();
}
