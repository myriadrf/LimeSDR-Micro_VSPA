// SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
// Copyright 2023 - 2025  NXP

// =============================================================================
//! @file       chip-LA9310.h
//! @brief      LA9310 chip definitions.
//! @author     NXP Semiconductors.
// =============================================================================

#ifndef __CHIP_LA9310_H__
#define __CHIP_LA9310_H__

// -----------------------------------------------------------------------------
// Device memory map:
// LA9310 Architecture Definition, section 16.1.
// -----------------------------------------------------------------------------

#define BASE_PCIE1_TARGET 0xA0000000 //!< PCIe #1 target.
#define BASE_AXIQ_L0 0x44000000      //!< Low-speed AXIQ interface #0.
#define BASE_PCIE1_MSI 0x5FFF0000    //!< PCIe #1 MSI.
#define BASE_DTCM 0x20000000         //!< Shared DTCM
#define BASE_VSPA0 0x20400000        //!< VSPA-0 AXI slave interface.
#define BASE_CCSR 0x40000000         //!< Configuration, Control and Status Register address space.
#define BASE_LLCP 0x44010000         //!< LLCP base address.

#define CCSR_BASE_VSPA (BASE_CCSR + 0x1000000)        /**< VSPA base address.                             */
#define CCSR_BASE_PHY_TIMER (BASE_CCSR + 0x1020000)   /**< PHY Timer base address.                        */
#define CCSR_BASE_ADC_DAC (BASE_CCSR + 0x1040000)     /**< ADC/DAC base address.                          */
#define CCSR_BASE_AEM (BASE_CCSR + 0x1200000)         /**< Address Error Manager base address.            */
#define CCSR_BASE_CGU (BASE_CCSR + 0x1360000)         /**< Platform Clock Generation Unit base address.   */
#define CCSR_BASE_CLOCK (BASE_CCSR + 0x1370000)       /**< Clocking registers base address.               */
#define CCSR_BASE_DCFG (BASE_CCSR + 0x1E00000)        /**< Device Configuration base address.             */
#define CCSR_BASE_PMU (BASE_CCSR + 0x1E30000)         /**< Power Management Unit base address.            */
#define CCSR_BASE_RESET (BASE_CCSR + 0x1E60000)       /**< Reset base address.                            */
#define CCSR_BASE_SERDES (BASE_CCSR + 0x1EA0000)      /**< SerDes base address.                           */
#define CCSR_BASE_TMU (BASE_CCSR + 0x1F80000)         /**< Thermal Monitoring Unit base address.          */
#define CCSR_BASE_MESSAGE (BASE_CCSR + 0x1FC0000)     /**< Message Unit base address.                     */
#define CCSR_BASE_PIN_CONTROL (BASE_CCSR + 0x1FF0000) /**< Pin Control base address.                      */
#define CCSR_BASE_I2C1 (BASE_CCSR + 0x2000000)        /**< I2C #1 base address.                           */
#define CCSR_BASE_I2C2 (BASE_CCSR + 0x2010000)        /**< I2C #2 base address.                           */
#define CCSR_BASE_SPI (BASE_CCSR + 0x2100000)         /**< SPI base address.                              */
#define CCSR_BASE_UART (BASE_CCSR + 0x21C0000)        /**< UART base address.                             */
#define CCSR_BASE_EDMA (BASE_CCSR + 0x22C0000)        /**< eDMA base address.                             */
#define CCSR_BASE_LLCP (BASE_CCSR + 0x22E0000)        /**< LLCP base address.                             */
#define CCSR_BASE_GPIO (BASE_CCSR + 0x2300000)        /**< GPIO base address.                             */
#define CCSR_BASE_WDT (BASE_CCSR + 0x23C0000)         /**< Watchdog Timer base address.                   */
#define CCSR_BASE_PCIE (BASE_CCSR + 0x3400000)        /**< PCIexpress Controller PF0 base address.        */
#define CCSR_BASE_PCIE_CTRL (BASE_CCSR + 0x34C0000)   /**< PCI express Controller PFO controls.           */

#define BASE_AXIQ(bank) (BASE_AXIQ_L0 + ((bank)*0x00010000))
#define BASE_VSPA(core) (BASE_VSPA0 + ((core)*0x00400000))

#define SIZE_DTCM (128 * 1024)                 /**< Data TCM byte size.            */
#define SIZE_PCIE1_TARGET (1024 * 1024 * 1024) /**< Host memory byte size.         */

// -----------------------------------------------------------------------------
// VSPA hardware parameters:
// -----------------------------------------------------------------------------

#define ATID_0 0

typedef enum { VSPA_ATID_0 = ATID_0, VSPA_ATID_CNT } vspa_atid_t;

#define VCPU_SIZE_PRAM (32 * 1024) /**< VCPU program memory byte size. */
#define VCPU_SIZE_DRAM (16 * 1024) /**< VCPU data memory byte size. */

#define IPPU_SIZE_PRAM (2 * 1024)  /**< IPPU program memory byte size. */
#define IPPU_SIZE_DRAM (16 * 1024) /**< IPPU data memory byte size. */

#define VSPA_SIZE_DRAM (VCPU_SIZE_DRAM + IPPU_SIZE_DRAM) //!< Total data memory size (byte).
#define VSPA_AXI_DW 128                                  //!< VSPA AXI width in bits.
#define VSPA_AXI_FIFO_DEPTH 32                           //!< VSPA AXI FIFO depth in # of elements (each VSPA_AXI_DW wide).

#define NAXILANE8 (VSPA_AXI_DW / 8)
#define NAXILANE16 (VSPA_AXI_DW / 16)
#define NAXILANE32 (VSPA_AXI_DW / 32)

#define NAXIVEC8(n) (((n) + NAXILANE8 - 1) / NAXILANE8)
#define NAXIVEC16(n) (((n) + NAXILANE16 - 1) / NAXILANE16)
#define NAXIVEC32(n) (((n) + NAXILANE32 - 1) / NAXILANE32)

#if defined(__VSPA3__)
#define __align_axi __attribute__((aligned(VSPA_AXI_DW / 8)))
#elif defined(__VSPA2__)
#define __align_axi __attribute__((aligned(VSPA_AXI_DW / 16)))
#else
#error "Architecture not supported!"
#endif

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

#define DMAC_CHAN_AXIQ_L0_RX0 DMAC_CHAN_1
#define DMAC_CHAN_AXIQ_L0_RX1 DMAC_CHAN_2
#define DMAC_CHAN_AXIQ_L0_RX2 DMAC_CHAN_3
#define DMAC_CHAN_AXIQ_L0_RX3 DMAC_CHAN_4
#define DMAC_CHAN_AXIQ_L0_TX0 DMAC_CHAN_11

#define DMAC_MASK_AXIQ_L0_RX0 (0b1 << DMAC_CHAN_AXIQ_L0_RX0)
#define DMAC_MASK_AXIQ_L0_RX1 (0b1 << DMAC_CHAN_AXIQ_L0_RX1)
#define DMAC_MASK_AXIQ_L0_RX2 (0b1 << DMAC_CHAN_AXIQ_L0_RX2)
#define DMAC_MASK_AXIQ_L0_RX3 (0b1 << DMAC_CHAN_AXIQ_L0_RX3)
#define DMAC_MASK_AXIQ_L0_TX0 (0b1 << DMAC_CHAN_AXIQ_L0_TX0)

// -----------------------------------------------------------------------------
// GPIO assignment:
// -----------------------------------------------------------------------------

#define GPIO_AXIQ_CR0 4 //!< Low-speed AXIQ #0 control register 0 (GPOUT).
#define GPIO_AXIQ_CR1 5 //!< Low-speed AXIQ #0 control register 1 (GPOUT).
#define GPIO_AXIQ_CR2 6 //!< Low-speed AXIQ #0 control register 2 (GPOUT).
#define GPIO_AXIQ_CR3 7 //!< Low-speed AXIQ #0 control register 3 (GPOUT).
#define GPIO_AXIQ_SR0 0 //!< Low-speed AXIQ #0 status register 0 (GPIN).
#define GPIO_AXIQ_SR1 1 //!< Low-speed AXIQ #0 status register 1 (GPIN).
#define GPIO_AXIQ_SR2 2 //!< Low-speed AXIQ #0 status register 2 (GPIN).
#define GPIO_AXIQ_SR3 3 //!< Low-speed AXIQ #0 status register 3 (GPIN).

#endif // __CHIP_LA9310_H__
