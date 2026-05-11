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

#include "dma_common.h"
#include "platform.h"

static bool first_run = true;

// volatile uint32_t mailbox_out_msg_0_MSB = 0; // (VCPU_OUT_0_MSB)
// volatile uint32_t mailbox_out_msg_0_LSB = 0; // (VCPU_OUT_0_LSB)
static inline void HostMBox0Post(uint64_t msg64) {
    // mailbox_out_msg_0_MSB = HIWORD(msg64);
    // mailbox_out_msg_0_LSB = LOWORD(msg64);
    host_mbox0_post(msg64);
}

static void setup(void) {
    uint64_t msg64 = 0;

    vcpu_swver(0x00020002);
    ccnt_enable();
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

    first_run = false;

    memclr(&player_state, sizeof(player_state));
}

__attribute__((noreturn)) void SwReset(void) __noreturn {
    extern void start(void);
    entry(start);
    host_clear();
    HostMBox0Post(MAKEDWORD(0x0, 0x1));
    __idle();
}

static void HandleCommand(uint64_t msg64) {
    const mbox_opc_e op_code = (mbox_opc_e)((HIWORD(msg64) & 0xFF000000) >> 24);
    const uint32_t msg_msb = HIWORD(msg64);
    const uint32_t msg_lsb = LOWORD(msg64);

    const e_rx_channel rx_index = (e_rx_channel)((msg_msb & 0x00300000) >> 20);
    const uint32_t fifo_size = (msg_msb & 0x0000FFFF) * 4096;
    const uint32_t fifo_addr = msg_lsb;

    switch (op_code) {
    case MBOX_OPC_TX_HOST_FIFO_CONFIG: {
        TxConfigureHostFIFO(fifo_addr, fifo_size);
        HostMBox0Post(MAKEDWORD(op_code, lime_Result_Success));
        break;
    }
    case MBOX_OPC_RX_HOST_FIFO_CONFIG: {
        RxHostFIFO(rx_index, fifo_addr, fifo_size);
        HostMBox0Post(MAKEDWORD(op_code, lime_Result_Success));
        break;
    }
    case MBOX_OPC_TX_CONFIGURE: {
        const uint32_t interpolation = 1 << (msg_lsb & 0x3);
        HostMBox0Post(MAKEDWORD(op_code, TxConfigure(interpolation)));
        break;
    }
    case MBOX_OPC_RX_CONFIGURE: {
        const uint32_t decimation = 1 << (msg_lsb & 0x3);
        HostMBox0Post(MAKEDWORD(op_code, RxChannelConfigure(rx_index, decimation)));
        break;
    }
    case MBOX_OPC_TX_CONTROL:
        HostMBox0Post(MAKEDWORD(op_code, TxDDR_control(msg64)));
        break;
    case MBOX_OPC_RX_CONTROL:
        HostMBox0Post(MAKEDWORD(op_code, RxDDR_control(rx_index, msg64)));
        break;
    case MBOX_OPC_TX_BURST_LENGTH:
        TxSetBurstSize(msg_lsb);
        HostMBox0Post(MAKEDWORD(0, 0x1));
        break;

    case MBOX_OPC_SINGLE_TONE_TX:
        TxTone_control(msg64);
        HostMBox0Post(MAKEDWORD(0, 0x1));
        break;

    case MBOX_OPC_SINGLE_TONE_RX:
        // RX_single_tone_measurement();
        break;

    case MBOX_OPC_RX_CHAN_SELECT:
        RxChannelSelect(msg_lsb & 0xf);
        HostMBox0Post(MAKEDWORD(op_code, 0x1));
        break;

    case MBOX_OPC_DONE_SWRESET:
        SwReset();
        break;

    case MBOX_OPC_PROXY_OFFSET: {
        const uint32_t proxy_offset_read_only = ((msg_msb & 0x00100000) >> 20); /* bit 52 */
        if (!proxy_offset_read_only)
            dmem_proxy_set_offset(msg_lsb);
        HostMBox0Post(MAKEDWORD(msg_lsb, 0x1));
        break;
    }

    case MBOX_OPC_TX_AXIQ: {
        if (msg_lsb) {
            // Enable Tx
            axiq_fifo_tx_enable(AXIQ_BANK_0, AXIQ_FIFO_TX0);

            // clear error
            axiq_fifo_tx_cr(AXIQ_BANK_0, AXIQ_FIFO_TX0, AXIQ_CR_CLRERR, AXIQ_CR_CLRERR);
            axiq_fifo_tx_cr(AXIQ_BANK_0, AXIQ_FIFO_TX0, AXIQ_CR_CLRERR, 0);
        } else
            axiq_tx_disable(); // enters flush mode?
        HostMBox0Post(MAKEDWORD(msg_lsb, 0x1));
        break;
    }

    case MBOX_OPC_TX_PTR_RST: {
        stream_write_ptr_rst(TX_DAC_WR_DMA_CHANNEL, 0x4400B000);
        HostMBox0Post(MAKEDWORD(msg_lsb, 0x1));
        break;
    }

    case MBOX_OPC_RX_PREPARE: {
        HostMBox0Post(MAKEDWORD(op_code, RxPrepare()));
    }

    default:
        // not a valid command, NACK
        HostMBox0Post(MAKEDWORD(0x0, 0x0));
        break;
    }
}

//----------------------------------------------------------------------------------------------------
__attribute__((noreturn)) void main(void) {
    l1_trace_clear();

    l1_trace(L1_TRACE_MSG_ENTRY_MAIN, (uint32_t)iord(CONTROL));
    l1_trace(L1_TRACE_MSG_ENTRY_MAIN, (uint32_t)iord(DMA_GO_STAT));
    l1_trace(L1_TRACE_MSG_ENTRY_MAIN, (uint32_t)iord(DMA_CFGERR_STAT));
    l1_trace(L1_TRACE_MSG_ENTRY_MAIN, (uint32_t)iord(DMA_COMP_STAT));

    if (first_run) {
        // !! this workaround is needed otherwise VSPA gets stuck ( dmac_is_available()/dmac_is_complete() doesn't work)
        // !! unless issue "ccs::config_chain {la9310 dap}" on ccs
        const uint16_t HOST_VCPU_A011455 = (0x024 >> 2);
        iowr(HOST_VCPU_A011455, 0x10, 0x10);
        setup();

        // update host vspa_dmem_proxy
        player_state.info.dmemProxyOffset = (uint32_t)&player_state;
        player_state.info.l1_trace_offset = 0;
        player_state.info.l1_trace_size = L1_TRACE_COUNT * 16;

        InitializeRx();
        InitializeTx();
    }

    while (1) {
        if (vspa_mbox0_is_valid()) // message from Host
        {
            HandleCommand(host_mbox0_read());
            host_mbox0_clear();
        }
        if (vspa_mbox1_is_valid()) // message from Arm M4
        {
            HandleCommand(host_mbox1_read());
            host_mbox1_clear();
        }

        ProcessTx();
        ProcessRx();
    }
    __builtin_done();
}
