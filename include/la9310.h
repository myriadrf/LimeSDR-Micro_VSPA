/* SPDX-License-Identifier: BSD-3-Clause */

/*
 * Copyright 2024 NXP
 */

#ifndef REDCAP_H_
#define REDCAP_H_

/**
 * DMA channel assignment: Overlay Loading
 */
#define REDCAP_DMAC_CHAN_OVLY DMAC_CHAN_0
#define REDCAP_DMAC_MASK_OVLY (0b1 << REDCAP_DMAC_CHAN_OVLY)

/**
 * @defgroup    GROUP_LA9310_DMA   DMA Channel Allocation
 * @{
 */

#define LA9310_DMA_CHAN_OVERLAY 0    /**< DMA channel for Overlay                            */
#define LA9310_DMA_CHAN_AXIQ_RX1 1   /**< DMA channel for AXIQ receive channel A.            */
#define LA9310_DMA_CHAN_AXIQ_RX2 2   /**< DMA channel for AXIQ receive channel B.            */
#define LA9310_DMA_CHAN_AXIQ_RX3 3   /**< DMA channel for AXIQ receive channel C.            */
#define LA9310_DMA_CHAN_AXIQ_RX4 4   /**< DMA channel for AXIQ receive channel D.            */
#define LA9310_DMA_CHAN_AXIQ_RX5 5   /**< DMA channel for AXIQ auxiliary receive channel.    */
#define LA9310_DMA_CHAN_AXIQ_RX6 6   /**< DMA channel for AXIQ RSSI channel.                 */
#define LA9310_DMA_CHAN_AXIQ_TX5 11  /**< DMA channel for AXIQ transmit channel.             */
#define LA9310_DMA_CHAN_HOST_RD 12   /**< DMA channel for data generation.                   */
#define LA9310_DMA_CHAN_GAIN_CMD 13  /**< DMA channel for gain update command.               */
#define LA9310_DMA_CHAN_PHY_TIMER 14 /**< DMA channel for PHY timer configuration/control.   */
#define LA9310_DMA_CHAN_HOST_WR 15   /**< DMA channel to upload decoded data from MFI task.  */

#define LA9310_DMA_MASK_OVERLAY (0x1 << LA9310_DMA_CHAN_OVERLAY)     /**< Bit mask for Overlay                             */
#define LA9310_DMA_MASK_AXIQ_RX1 (0x1 << LA9310_DMA_CHAN_AXIQ_RX1)   /**< Bit mask for AXIQ receive channel A.           */
#define LA9310_DMA_MASK_AXIQ_RX2 (0x1 << LA9310_DMA_CHAN_AXIQ_RX2)   /**< Bit mask for AXIQ receive channel B.           */
#define LA9310_DMA_MASK_AXIQ_RX3 (0x1 << LA9310_DMA_CHAN_AXIQ_RX3)   /**< Bit mask for AXIQ receive channel C.           */
#define LA9310_DMA_MASK_AXIQ_RX4 (0x1 << LA9310_DMA_CHAN_AXIQ_RX4)   /**< Bit mask for AXIQ receive channel D.           */
#define LA9310_DMA_MASK_AXIQ_RX5 (0x1 << LA9310_DMA_CHAN_AXIQ_RX5)   /**< Bit mask for AXIQ auxiliary receive channel.   */
#define LA9310_DMA_MASK_AXIQ_RX6 (0x1 << LA9310_DMA_CHAN_AXIQ_RX6)   /**< Bit mask for AXIQ RSSI channel.                */
#define LA9310_DMA_MASK_AXIQ_TX5 (0x1 << LA9310_DMA_CHAN_AXIQ_TX5)   /**< Bit mask for AXIQ transmit channel.            */
#define LA9310_DMA_MASK_HOST_RD (0x1 << LA9310_DMA_CHAN_HOST_RD)     /**< Bit mask for data generation.                  */
#define LA9310_DMA_MASK_GAIN_CMD (0x1 << LA9310_DMA_CHAN_GAIN_CMD)   /**< Bit mask for gain update command.              */
#define LA9310_DMA_MASK_PHY_TIMER (0x1 << LA9310_DMA_CHAN_PHY_TIMER) /**< Bit mask for PHY timer configuration/control.  */
#define LA9310_DMA_MASK_HOST_WR (0x1 << LA9310_DMA_CHAN_HOST_WR)     /**< Bit mask for decoded data from MFI task.       */

#endif /* REDCAP_H_ */
