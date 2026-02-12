// SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
// Copyright 2018 - 2025   NXP Semiconductors

// =============================================================================
//! @file       chip-la1224.h
//! @brief      LA1224 chip definitions.
//! @author     NXP Semiconductors.
// =============================================================================

#ifndef __CHIP_LA1224_H__
#define __CHIP_LA1224_H__

// -----------------------------------------------------------------------------
// Device memory map:
// LA1224 Architecture Definition, section 16.1.
// -----------------------------------------------------------------------------

#define BASE_PCIE1_TARGET 0x00000000 //!< PCIe #1 target.
#define BASE_PCIE2_TARGET 0x80000000 //!< PCIe #2 target.
#define BASE_AXIQ_HS 0xD2000000      //!< High-speed AXIQ interface.
#define BASE_AXIQ_L0 0xD2010000      //!< Low-speed AXIQ interface #0.
#define BASE_AXIQ_L1 0xD2020000      //!< Low-speed AXIQ interface #0.
#define BASE_PCIE2_MSI 0xDFFE0000    //!< PCIe #2 MSI.
#define BASE_PCIE1_MSI 0xDFFF0000    //!< PCIe #1 MSI.
#define BASE_SRAM 0xE01F0000         //!< Shared SRAM.
#define BASE_PEBM 0xE0200000         //!< PEB memory.
#define BASE_VSPA0 0xE1000000        //!< VSPA-0 AXI slave interface.
#define BASE_VSPA1 0xE1400000        //!< VSPA-1 AXI slave interface.
#define BASE_VSPA2 0xE1800000        //!< VSPA-2 AXI slave interface.
#define BASE_VSPA3 0xE1C00000        //!< VSPA-3 AXI slave interface.
#define BASE_VSPA4 0xE2000000        //!< VSPA-4 AXI slave interface.
#define BASE_VSPA5 0xE2400000        //!< VSPA-5 AXI slave interface.
#define BASE_VSPA6 0xE2800000        //!< VSPA-6 AXI slave interface.
#define BASE_VSPA7 0xE2C00000        //!< VSPA-7 AXI slave interface.
#define BASE_FECA 0xE5000000         //!< FECA AXI slave interface.
#define BASE_CCSR 0xF8000000         //!< Configuration, Control and Status Register address space.
#define BASE_DCSR 0xFC000000         //!< Debug Control and Status Register address space.

#define BASE_AXIQ(bank) (BASE_AXIQ_L0 + ((bank)*0x00010000))
#define BASE_VSPA(core) (BASE_VSPA0 + ((core)*0x00400000))

#define SIZE_PEBM (1024 * 1024 * 2) //!< 2MiB.

// -----------------------------------------------------------------------------
// VSPA hardware parameters:
// -----------------------------------------------------------------------------

#define ATID_0 0
#define ATID_1 1
#define ATID_2 2
#define ATID_3 3
#define ATID_4 4
#define ATID_5 5
#define ATID_6 6
#define ATID_7 7

#ifndef __ASSEMBLER__

typedef enum {
    VSPA_ATID_0 = ATID_0,
    VSPA_ATID_1 = ATID_1,
    VSPA_ATID_2 = ATID_2,
    VSPA_ATID_3 = ATID_3,
    VSPA_ATID_4 = ATID_4,
    VSPA_ATID_5 = ATID_5,
    VSPA_ATID_6 = ATID_6,
    VSPA_ATID_7 = ATID_7,
    VSPA_ATID_CNT
} vspa_atid_t;

#endif

#define VCPU_SIZE_PRAM (1024 * 128)                      //!< VCPU program memory size (byte).
#define VCPU_SIZE_DRAM (1024 * 128)                      //!< VCPU data bank memory size (byte).
#define IPPU_SIZE_PRAM (1024 * 16)                       //!< IPPU program memory size (byte).
#define IPPU_SIZE_DRAM (1024 * 192)                      //!< IPPU data bank memory size (byte).
#define VSPA_SIZE_DRAM (VCPU_SIZE_DRAM + IPPU_SIZE_DRAM) //!< Total data memory size (byte).
#define VSPA_AXI_DW 512                                  //!< VSPA AXI width in bits.
#define VSPA_AXI_FIFO_DEPTH 32                           //!< VSPA AXI FIFO depth in # of elements (each VSPA_AXI_DW wide).

#define NAXILANE8 (VSPA_AXI_DW / 8)
#define NAXILANE16 (VSPA_AXI_DW / 16)
#define NAXILANE32 (VSPA_AXI_DW / 32)

#define NAXIVEC8(n) (((n) + NAXILANE8 - 1) / NAXILANE8)
#define NAXIVEC16(n) (((n) + NAXILANE16 - 1) / NAXILANE16)
#define NAXIVEC32(n) (((n) + NAXILANE32 - 1) / NAXILANE32)

#define __align_axi __attribute__((aligned(VSPA_AXI_DW / 8)))

#define BASE_VSPA_VCPU(core) (BASE_VSPA(core) + 0x00000000)          // Access VCPU DMEM.
#define BASE_VSPA_IPPU(core) (BASE_VSPA(core) + 0x00100000)          // Access IPPU DMEM.
#define BASE_VSPA_FLAG(core) (BASE_VSPA(core) + 0x00200000)          // Access AXI slave flags.
#define BASE_VSPA_DMEM(core) (BASE_VSPA_IPPU(core) - VCPU_SIZE_DRAM) // Access VCPU+IPPU DMEM in a contiguous region.

// -----------------------------------------------------------------------------
// DMA channel assignment:
// -----------------------------------------------------------------------------

#define DMAC_CHAN_0 0   //!< High-priority DMA channel.
#define DMAC_CHAN_1 1   //!< DMA channel #1 .
#define DMAC_CHAN_2 2   //!< DMA channel #2 .
#define DMAC_CHAN_3 3   //!< DMA channel #3 .
#define DMAC_CHAN_4 4   //!< DMA channel #4 .
#define DMAC_CHAN_5 5   //!< DMA channel #5 .
#define DMAC_CHAN_6 6   //!< DMA channel #6 .
#define DMAC_CHAN_7 7   //!< DMA channel #7 .
#define DMAC_CHAN_8 8   //!< DMA channel #8 .
#define DMAC_CHAN_9 9   //!< DMA channel #9 .
#define DMAC_CHAN_10 10 //!< DMA channel #10.
#define DMAC_CHAN_11 11 //!< DMA channel #11.
#define DMAC_CHAN_12 12 //!< DMA channel #12.
#define DMAC_CHAN_13 13 //!< DMA channel #13.
#define DMAC_CHAN_14 14 //!< DMA channel #14.
#define DMAC_CHAN_15 15 //!< DMA channel #15.
#define DMAC_CHAN_16 16 //!< DMA channel #16.
#define DMAC_CHAN_17 17 //!< DMA channel #17.
#define DMAC_CHAN_18 18 //!< DMA channel #18.
#define DMAC_CHAN_19 19 //!< DMA channel #19.
#define DMAC_CHAN_20 20 //!< DMA channel #20.
#define DMAC_CHAN_21 21 //!< DMA channel #21.
#define DMAC_CHAN_22 22 //!< DMA channel #22.
#define DMAC_CHAN_23 23 //!< DMA channel #23.
#define DMAC_CHAN_24 24 //!< DMA channel #24.
#define DMAC_CHAN_25 25 //!< DMA channel #25.
#define DMAC_CHAN_26 26 //!< DMA channel #26.
#define DMAC_CHAN_27 27 //!< DMA channel #27.
#define DMAC_CHAN_28 28 //!< DMA channel #28.
#define DMAC_CHAN_29 29 //!< DMA channel #29.
#define DMAC_CHAN_30 30 //!< DMA channel #30.
#define DMAC_CHAN_31 31 //!< DMA channel #31.

#define DMAC_MASK_0 (0b1 << DMAC_CHAN_0)
#define DMAC_MASK_1 (0b1 << DMAC_CHAN_1)
#define DMAC_MASK_2 (0b1 << DMAC_CHAN_2)
#define DMAC_MASK_3 (0b1 << DMAC_CHAN_3)
#define DMAC_MASK_4 (0b1 << DMAC_CHAN_4)
#define DMAC_MASK_5 (0b1 << DMAC_CHAN_5)
#define DMAC_MASK_6 (0b1 << DMAC_CHAN_6)
#define DMAC_MASK_7 (0b1 << DMAC_CHAN_7)
#define DMAC_MASK_8 (0b1 << DMAC_CHAN_8)
#define DMAC_MASK_9 (0b1 << DMAC_CHAN_9)
#define DMAC_MASK_10 (0b1 << DMAC_CHAN_10)
#define DMAC_MASK_11 (0b1 << DMAC_CHAN_11)
#define DMAC_MASK_12 (0b1 << DMAC_CHAN_12)
#define DMAC_MASK_13 (0b1 << DMAC_CHAN_13)
#define DMAC_MASK_14 (0b1 << DMAC_CHAN_14)
#define DMAC_MASK_15 (0b1 << DMAC_CHAN_15)
#define DMAC_MASK_16 (0b1 << DMAC_CHAN_16)
#define DMAC_MASK_17 (0b1 << DMAC_CHAN_17)
#define DMAC_MASK_18 (0b1 << DMAC_CHAN_18)
#define DMAC_MASK_19 (0b1 << DMAC_CHAN_19)
#define DMAC_MASK_20 (0b1 << DMAC_CHAN_20)
#define DMAC_MASK_21 (0b1 << DMAC_CHAN_21)
#define DMAC_MASK_22 (0b1 << DMAC_CHAN_22)
#define DMAC_MASK_23 (0b1 << DMAC_CHAN_23)
#define DMAC_MASK_24 (0b1 << DMAC_CHAN_24)
#define DMAC_MASK_25 (0b1 << DMAC_CHAN_25)
#define DMAC_MASK_26 (0b1 << DMAC_CHAN_26)
#define DMAC_MASK_27 (0b1 << DMAC_CHAN_27)
#define DMAC_MASK_28 (0b1 << DMAC_CHAN_28)
#define DMAC_MASK_29 (0b1 << DMAC_CHAN_29)
#define DMAC_MASK_30 (0b1 << DMAC_CHAN_30)
#define DMAC_MASK_31 (0b1 << DMAC_CHAN_31)

#define DMAC_CHAN_AXIQ_L0_RX0 DMAC_CHAN_1
#define DMAC_CHAN_AXIQ_L0_RX1 DMAC_CHAN_2
#define DMAC_CHAN_AXIQ_L0_RX2 DMAC_CHAN_3
#define DMAC_CHAN_AXIQ_L0_TX0 DMAC_CHAN_4
#define DMAC_CHAN_AXIQ_L0_TX1 DMAC_CHAN_5
#define DMAC_CHAN_AXIQ_L1_RX0 DMAC_CHAN_6
#define DMAC_CHAN_AXIQ_L1_RX1 DMAC_CHAN_7
#define DMAC_CHAN_AXIQ_L1_RX2 DMAC_CHAN_8
#define DMAC_CHAN_AXIQ_L1_TX0 DMAC_CHAN_9
#define DMAC_CHAN_AXIQ_L1_TX1 DMAC_CHAN_10
#define DMAC_CHAN_AXIQ_HS_RX0 DMAC_CHAN_11
#define DMAC_CHAN_AXIQ_HS_RX1 DMAC_CHAN_12
#define DMAC_CHAN_AXIQ_HS_TX0 DMAC_CHAN_14
#define DMAC_CHAN_AXIQ_HS_TX1 DMAC_CHAN_15
#define DMAC_CHAN_CHAIN0 DMAC_CHAN_16
#define DMAC_CHAN_CHAIN1 DMAC_CHAN_17
#define DMAC_CHAN_CHAIN2 DMAC_CHAN_18
#define DMAC_CHAN_CHAIN3 DMAC_CHAN_19
#define DMAC_CHAN_CHAIN4 DMAC_CHAN_20
#define DMAC_CHAN_CHAIN5 DMAC_CHAN_21
#define DMAC_CHAN_CHAIN6 DMAC_CHAN_22
#define DMAC_CHAN_CHAIN7 DMAC_CHAN_23

#define DMAC_MASK_AXIQ_L0_RX0 (0b1 << DMAC_CHAN_AXIQ_L0_RX0)
#define DMAC_MASK_AXIQ_L0_RX1 (0b1 << DMAC_CHAN_AXIQ_L0_RX1)
#define DMAC_MASK_AXIQ_L0_RX2 (0b1 << DMAC_CHAN_AXIQ_L0_RX2)
#define DMAC_MASK_AXIQ_L0_TX0 (0b1 << DMAC_CHAN_AXIQ_L0_TX0)
#define DMAC_MASK_AXIQ_L0_TX1 (0b1 << DMAC_CHAN_AXIQ_L0_TX1)
#define DMAC_MASK_AXIQ_L1_RX0 (0b1 << DMAC_CHAN_AXIQ_L1_RX0)
#define DMAC_MASK_AXIQ_L1_RX1 (0b1 << DMAC_CHAN_AXIQ_L1_RX1)
#define DMAC_MASK_AXIQ_L1_RX2 (0b1 << DMAC_CHAN_AXIQ_L1_RX2)
#define DMAC_MASK_AXIQ_L1_TX0 (0b1 << DMAC_CHAN_AXIQ_L1_TX0)
#define DMAC_MASK_AXIQ_L1_TX1 (0b1 << DMAC_CHAN_AXIQ_L1_TX1)
#define DMAC_MASK_AXIQ_HS_RX0 (0b1 << DMAC_CHAN_AXIQ_HS_RX0)
#define DMAC_MASK_AXIQ_HS_RX1 (0b1 << DMAC_CHAN_AXIQ_HS_RX1)
#define DMAC_MASK_AXIQ_HS_TX0 (0b1 << DMAC_CHAN_AXIQ_HS_TX0)
#define DMAC_MASK_AXIQ_HS_TX1 (0b1 << DMAC_CHAN_AXIQ_HS_TX1)
#define DMAC_MASK_CHAIN0 (0b1 << DMAC_CHAN_CHAIN0)
#define DMAC_MASK_CHAIN1 (0b1 << DMAC_CHAN_CHAIN1)
#define DMAC_MASK_CHAIN2 (0b1 << DMAC_CHAN_CHAIN2)
#define DMAC_MASK_CHAIN3 (0b1 << DMAC_CHAN_CHAIN3)
#define DMAC_MASK_CHAIN4 (0b1 << DMAC_CHAN_CHAIN4)
#define DMAC_MASK_CHAIN5 (0b1 << DMAC_CHAN_CHAIN5)
#define DMAC_MASK_CHAIN6 (0b1 << DMAC_CHAN_CHAIN6)
#define DMAC_MASK_CHAIN7 (0b1 << DMAC_CHAN_CHAIN7)

// -----------------------------------------------------------------------------
// External event assignment:
// -----------------------------------------------------------------------------

#define EXT_CHAN_VSPA0 0
#define EXT_CHAN_VSPA1 1
#define EXT_CHAN_VSPA2 2
#define EXT_CHAN_VSPA3 3
#define EXT_CHAN_VSPA4 4
#define EXT_CHAN_VSPA5 5
#define EXT_CHAN_VSPA6 6
#define EXT_CHAN_VSPA7 7
#define EXT_CHAN_TBGEN1_0_2_4_6 8
#define EXT_CHAN_TBGEN1_8_10_1_3 8
#define EXT_CHAN_TBGEN1_1_3_5_7 9
#define EXT_CHAN_TBGEN1_9_0_2_4 9
#define EXT_CHAN_TBGEN2_0_2_4_6 10
#define EXT_CHAN_TBGEN2_8_10_1_3 10
#define EXT_CHAN_TBGEN2_1_3_5_7 11
#define EXT_CHAN_TBGEN2_9_0_2_4 11
#define EXT_CHAN_SOC_GPIO2 12
#define EXT_CHAN_SOC_GPIO3 13
#define EXT_CHAN_SOC_GPIO4 14
#define EXT_CHAN_AXIQ_L0_TX0 15
#define EXT_CHAN_AXIQ_L0_TX1 16
#define EXT_CHAN_AXIQ_L0_RX0 17
#define EXT_CHAN_AXIQ_L0_RX1 18
#define EXT_CHAN_AXIQ_L1_TX0 19
#define EXT_CHAN_AXIQ_L1_TX1 20
#define EXT_CHAN_AXIQ_L1_RX0 21
#define EXT_CHAN_AXIQ_L1_RX1 22
#define EXT_CHAN_AXIQ_HS_TX0 23
#define EXT_CHAN_AXIQ_HS_TX1 24
#define EXT_CHAN_AXIQ_HS_RX0 25
#define EXT_CHAN_AXIQ_HS_RX1 26

#define EXT_MASK_VSPA0 (0b1 << EXT_CHAN_VSPA0)
#define EXT_MASK_VSPA1 (0b1 << EXT_CHAN_VSPA1)
#define EXT_MASK_VSPA2 (0b1 << EXT_CHAN_VSPA2)
#define EXT_MASK_VSPA3 (0b1 << EXT_CHAN_VSPA3)
#define EXT_MASK_VSPA4 (0b1 << EXT_CHAN_VSPA4)
#define EXT_MASK_VSPA5 (0b1 << EXT_CHAN_VSPA5)
#define EXT_MASK_VSPA6 (0b1 << EXT_CHAN_VSPA6)
#define EXT_MASK_VSPA7 (0b1 << EXT_CHAN_VSPA7)
#define EXT_MASK_TBGEN1_0_2_4_6 (0b1 << EXT_CHAN_TBGEN1_0_2_4_6)
#define EXT_MASK_TBGEN1_8_10_1_3 (0b1 << EXT_CHAN_TBGEN1_8_10_1_3)
#define EXT_MASK_TBGEN1_1_3_5_7 (0b1 << EXT_CHAN_TBGEN1_1_3_5_7)
#define EXT_MASK_TBGEN1_9_0_2_4 (0b1 << EXT_CHAN_TBGEN1_9_0_2_4)
#define EXT_MASK_TBGEN2_0_2_4_6 (0b1 << EXT_CHAN_TBGEN2_0_2_4_6)
#define EXT_MASK_TBGEN2_8_10_1_3 (0b1 << EXT_CHAN_TBGEN2_8_10_1_3)
#define EXT_MASK_TBGEN2_1_3_5_7 (0b1 << EXT_CHAN_TBGEN2_1_3_5_7)
#define EXT_MASK_TBGEN2_9_0_2_4 (0b1 << EXT_CHAN_TBGEN2_9_0_2_4)
#define EXT_MASK_SOC_GPIO2 (0b1 << EXT_CHAN_SOC_GPIO2)
#define EXT_MASK_SOC_GPIO3 (0b1 << EXT_CHAN_SOC_GPIO3)
#define EXT_MASK_SOC_GPIO4 (0b1 << EXT_CHAN_SOC_GPIO4)
#define EXT_MASK_AXIQ_L0_TX0 (0b1 << EXT_CHAN_AXIQ_L0_TX0)
#define EXT_MASK_AXIQ_L0_TX1 (0b1 << EXT_CHAN_AXIQ_L0_TX1)
#define EXT_MASK_AXIQ_L0_RX0 (0b1 << EXT_CHAN_AXIQ_L0_RX0)
#define EXT_MASK_AXIQ_L0_RX1 (0b1 << EXT_CHAN_AXIQ_L0_RX1)
#define EXT_MASK_AXIQ_L1_TX0 (0b1 << EXT_CHAN_AXIQ_L1_TX0)
#define EXT_MASK_AXIQ_L1_TX1 (0b1 << EXT_CHAN_AXIQ_L1_TX1)
#define EXT_MASK_AXIQ_L1_RX0 (0b1 << EXT_CHAN_AXIQ_L1_RX0)
#define EXT_MASK_AXIQ_L1_RX1 (0b1 << EXT_CHAN_AXIQ_L1_RX1)
#define EXT_MASK_AXIQ_HS_TX0 (0b1 << EXT_CHAN_AXIQ_HS_TX0)
#define EXT_MASK_AXIQ_HS_TX1 (0b1 << EXT_CHAN_AXIQ_HS_TX1)
#define EXT_MASK_AXIQ_HS_RX0 (0b1 << EXT_CHAN_AXIQ_HS_RX0)
#define EXT_MASK_AXIQ_HS_RX1 (0b1 << EXT_CHAN_AXIQ_HS_RX1)

// -----------------------------------------------------------------------------
// GPIO assignment:
// -----------------------------------------------------------------------------

#define GPIO_VSPA0 0        //!< VSPA0 GPIO.
#define GPIO_VSPA1 1        //!< VSPA1 GPIO.
#define GPIO_VSPA2 2        //!< VSPA2 GPIO.
#define GPIO_VSPA3 3        //!< VSPA3 GPIO.
#define GPIO_VSPA4 4        //!< VSPA4 GPIO.
#define GPIO_VSPA5 5        //!< VSPA5 GPIO.
#define GPIO_VSPA6 6        //!< VSPA6 GPIO.
#define GPIO_VSPA7 7        //!< VSPA7 GPIO.
#define GPIO_AXIQ_L0_CR0 8  //!< Low-speed AXIQ #0 control register 0 (GPOUT).
#define GPIO_AXIQ_L0_SR0 8  //!< Low-speed AXIQ #0 status register 0 (GPIN).
#define GPIO_AXIQ_L0_CR1 9  //!< Low-speed AXIQ #0 control register 1 (GPOUT).
#define GPIO_AXIQ_L0_SR1 9  //!< Low-speed AXIQ #0 status register 1 (GPIN).
#define GPIO_AXIQ_L1_CR0 10 //!< Low-speed AXIQ #1 control register 0 (GPOUT).
#define GPIO_AXIQ_L0_SR2 10 //!< Low-speed AXIQ #0 status register 2 (GPIN).
#define GPIO_AXIQ_L1_CR1 11 //!< Low-speed AXIQ #1 control register 1 (GPOUT).
#define GPIO_AXIQ_L1_SR0 11 //!< Low-speed AXIQ #1 status register 0 (GPIN).
#define GPIO_AXIQ_HS_CR0 12 //!< High-speed AXIQ control register 0 (GPOUT).
#define GPIO_AXIQ_L1_SR1 12 //!< Low-speed AXIQ #1 status register 1 (GPIN).
#define GPIO_AXIQ_HS_CR1 13 //!< High-speed AXIQ control register 1 (GPOUT).
#define GPIO_AXIQ_L1_SR2 13 //!< Low-speed AXIQ #1 status register 2 (GPIN).
#define GPIO_HS_TIME_GPO 14 //!< SoC GPOUT.
#define GPIO_AXIQ_HS_SR0 14 //!< High-speed AXIQ status register 0 (GPIN).
#define GPIO_TBGEN 15       //!< TBGEN GPOUT (GPOUT bit #0 for TBGEN1, bit #1 for TBGEN2).
#define GPIO_DMA_MUX 15     //!< DMA daisy-chain multiplexer selection (VSPA2 GPOUT bits 23:16).
#define GPIO_AXIQ_HS_SR1 15 //!< High-speed AXIQ status register 1 (GPIN).
#define GPIO_LS_TIME_GPO 16 //!< SoC GPOUT.
#define GPIO_AXIQ_HS_SR2 16 //!< High-speed AXIQ status register 2 (GPIN).

#define GPIO_DMA_MUX_MASK (0xFF << 16) //!< 8 DMA channels.

#endif // __CHIP_LA1224_H__
