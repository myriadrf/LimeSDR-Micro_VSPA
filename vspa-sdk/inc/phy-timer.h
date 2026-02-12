// SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
// Copyright 2023 - 2025  NXP

// =============================================================================
//! @file       phy-timer.h
//! @brief      Phy Timer management header file
//! @author     NXP Semiconductors.
// =============================================================================
#ifndef __PHY_TIMER_H__
#define __PHY_TIMER_H__

/**
 * @addtogroup  GROUP_PHY_TIMER
 * @{
 */

/**
 * @defgroup    GROUP_PHY_TIMER_REG Register Interface
 * @{
 */

/**
 * @defgroup    GROUP_PHY_TIMER_REG_ADDR    Register Map
 * @{
 */

#define PHY_TMR_CTRL (CCSR_BASE_PHY_TIMER + 0x00)  /**< PHY Timer control register address. */
#define PHY_TMR_C0SC (CCSR_BASE_PHY_TIMER + 0x04)  /**< PHY Timer comparator # 0 status and control register address.  */
#define PHY_TMR_C0V (CCSR_BASE_PHY_TIMER + 0x08)   /**< PHY Timer comparator # 0 value register address.               */
#define PHY_TMR_C1SC (CCSR_BASE_PHY_TIMER + 0x0C)  /**< PHY Timer comparator # 1 status and control register address.  */
#define PHY_TMR_C1V (CCSR_BASE_PHY_TIMER + 0x10)   /**< PHY Timer comparator # 1 value register address.               */
#define PHY_TMR_C2SC (CCSR_BASE_PHY_TIMER + 0x14)  /**< PHY Timer comparator # 2 status and control register address.  */
#define PHY_TMR_C2V (CCSR_BASE_PHY_TIMER + 0x18)   /**< PHY Timer comparator # 2 value register address.               */
#define PHY_TMR_C3SC (CCSR_BASE_PHY_TIMER + 0x1C)  /**< PHY Timer comparator # 3 status and control register address.  */
#define PHY_TMR_C3V (CCSR_BASE_PHY_TIMER + 0x20)   /**< PHY Timer comparator # 3 value register address.               */
#define PHY_TMR_C4SC (CCSR_BASE_PHY_TIMER + 0x24)  /**< PHY Timer comparator # 4 status and control register address.  */
#define PHY_TMR_C4V (CCSR_BASE_PHY_TIMER + 0x28)   /**< PHY Timer comparator # 4 value register address.               */
#define PHY_TMR_C5SC (CCSR_BASE_PHY_TIMER + 0x2C)  /**< PHY Timer comparator # 5 status and control register address.  */
#define PHY_TMR_C5V (CCSR_BASE_PHY_TIMER + 0x30)   /**< PHY Timer comparator # 5 value register address.               */
#define PHY_TMR_C6SC (CCSR_BASE_PHY_TIMER + 0x34)  /**< PHY Timer comparator # 6 status and control register address.  */
#define PHY_TMR_C6V (CCSR_BASE_PHY_TIMER + 0x38)   /**< PHY Timer comparator # 6 value register address.               */
#define PHY_TMR_C7SC (CCSR_BASE_PHY_TIMER + 0x3C)  /**< PHY Timer comparator # 7 status and control register address.  */
#define PHY_TMR_C7V (CCSR_BASE_PHY_TIMER + 0x40)   /**< PHY Timer comparator # 7 value register address.               */
#define PHY_TMR_C8SC (CCSR_BASE_PHY_TIMER + 0x44)  /**< PHY Timer comparator # 8 status and control register address.  */
#define PHY_TMR_C8V (CCSR_BASE_PHY_TIMER + 0x48)   /**< PHY Timer comparator # 8 value register address.               */
#define PHY_TMR_C9SC (CCSR_BASE_PHY_TIMER + 0x4C)  /**< PHY Timer comparator # 9 status and control register address.  */
#define PHY_TMR_C9V (CCSR_BASE_PHY_TIMER + 0x50)   /**< PHY Timer comparator # 9 value register address.               */
#define PHY_TMR_C10SC (CCSR_BASE_PHY_TIMER + 0x54) /**< PHY Timer comparator #10 status and control register address.  */
#define PHY_TMR_C10V (CCSR_BASE_PHY_TIMER + 0x58)  /**< PHY Timer comparator #10 value register address.               */
#define PHY_TMR_C11SC (CCSR_BASE_PHY_TIMER + 0x5C) /**< PHY Timer comparator #11 status and control register address.  */
#define PHY_TMR_C11V (CCSR_BASE_PHY_TIMER + 0x60)  /**< PHY Timer comparator #11 value register address.               */
#define PHY_TMR_C12SC (CCSR_BASE_PHY_TIMER + 0x64) /**< PHY Timer comparator #12 status and control register address.  */
#define PHY_TMR_C12V (CCSR_BASE_PHY_TIMER + 0x68)  /**< PHY Timer comparator #12 value register address.               */
#define PHY_TMR_C13SC (CCSR_BASE_PHY_TIMER + 0x6C) /**< PHY Timer comparator #13 status and control register address.  */
#define PHY_TMR_C13V (CCSR_BASE_PHY_TIMER + 0x70)  /**< PHY Timer comparator #13 value register address.               */
#define PHY_TMR_C14SC (CCSR_BASE_PHY_TIMER + 0x74) /**< PHY Timer comparator #14 status and control register address.  */
#define PHY_TMR_C14V (CCSR_BASE_PHY_TIMER + 0x78)  /**< PHY Timer comparator #14 value register address.               */
#define PHY_TMR_C15SC (CCSR_BASE_PHY_TIMER + 0x7C) /**< PHY Timer comparator #15 status and control register address.  */
#define PHY_TMR_C15V (CCSR_BASE_PHY_TIMER + 0x80)  /**< PHY Timer comparator #15 value register address.               */
#define PHY_TMR_C16SC (CCSR_BASE_PHY_TIMER + 0x84) /**< PHY Timer comparator #16 status and control register address.  */
#define PHY_TMR_C16V (CCSR_BASE_PHY_TIMER + 0x88)  /**< PHY Timer comparator #16 value register address.               */
#define PHY_TMR_C17SC (CCSR_BASE_PHY_TIMER + 0x8C) /**< PHY Timer comparator #17 status and control register address.  */
#define PHY_TMR_C17V (CCSR_BASE_PHY_TIMER + 0x90)  /**< PHY Timer comparator #17 value register address.               */
#define PHY_TMR_C18SC (CCSR_BASE_PHY_TIMER + 0x94) /**< PHY Timer comparator #18 status and control register address.  */
#define PHY_TMR_C18V (CCSR_BASE_PHY_TIMER + 0x98)  /**< PHY Timer comparator #18 value register address.               */
#define PHY_TMR_C19SC (CCSR_BASE_PHY_TIMER + 0x9C) /**< PHY Timer comparator #19 status and control register address.  */
#define PHY_TMR_C19V (CCSR_BASE_PHY_TIMER + 0xA0)  /**< PHY Timer comparator #19 value register address.               */
#define PHY_TMR_C20SC (CCSR_BASE_PHY_TIMER + 0xA4) /**< PHY Timer comparator #20 status and control register address.  */
#define PHY_TMR_C20V (CCSR_BASE_PHY_TIMER + 0xA8)  /**< PHY Timer comparator #20 value register address.               */
#define PHY_TMR_C21SC (CCSR_BASE_PHY_TIMER + 0xAC) /**< PHY Timer comparator #21 status and control register address.  */
#define PHY_TMR_C21V (CCSR_BASE_PHY_TIMER + 0xB0)  /**< PHY Timer comparator #21 value register address.               */
#define PHY_TMR_C22SC (CCSR_BASE_PHY_TIMER + 0xB4) /**< PHY Timer comparator #22 status and control register address.  */
#define PHY_TMR_C22V (CCSR_BASE_PHY_TIMER + 0xB8)  /**< PHY Timer comparator #22 value register address.               */

/** @} */

/**
 * @defgroup    GROUP_PHY_TIMER_REG_DEF     Register Definitions
 * @{
 */

#define PHY_TMR_CTRL_SHIFT_EN 0
#define PHY_TMR_CTRL_SHIFT_SRST 4
#define PHY_TMR_CTRL_SHIFT_DIV 8

#define PHY_TMR_CTRL_FIELD_DIV (0x3F << PHY_TMR_CTRL_SHIFT_DIV)

#define PHY_TMR_CTRL_EN (0x01 << PHY_TMR_CTRL_SHIFT_EN)
#define PHY_TMR_CTRL_SRST (0x01 << PHY_TMR_CTRL_SHIFT_SRST)

#define PHY_TMR_CSC_SHIFT_CMP_TRIG 0
#define PHY_TMR_CSC_SHIFT_DIR_TRIG 2
#define PHY_TMR_CSC_SHIFT_CTE 4
#define PHY_TMR_CSC_SHIFT_CAP 5
#define PHY_TMR_CSC_SHIFT_CMPE 6
#define PHY_TMR_CSC_SHIFT_CIF 7
#define PHY_TMR_CSC_SHIFT_CMPCAP 8
#define PHY_TMR_CSC_SHIFT_TVAL 31

#define PHY_TMR_CSC_FIELD_CMP_TRIG (0x3 << PHY_TMR_CSC_SHIFT_CMP_TRIG)
#define PHY_TMR_CSC_FIELD_DIR_TRIG (0x3 << PHY_TMR_CSC_SHIFT_DIR_TRIG)

#define PHY_TMR_CSC_CMP_TRIG_NO_CHANGE (0x0 << PHY_TMR_CSC_SHIFT_CMP_TRIG)
#define PHY_TMR_CSC_CMP_TRIG_FORCE0 (0x1 << PHY_TMR_CSC_SHIFT_CMP_TRIG)
#define PHY_TMR_CSC_CMP_TRIG_FORCE1 (0x2 << PHY_TMR_CSC_SHIFT_CMP_TRIG)
#define PHY_TMR_CSC_CMP_TRIG_INVERT (0x3 << PHY_TMR_CSC_SHIFT_CMP_TRIG)

#define PHY_TMR_CSC_DIR_TRIG_NO_CHANGE (0x0 << PHY_TMR_CSC_SHIFT_DIR_TRIG)
#define PHY_TMR_CSC_DIR_TRIG_FORCE0 (0x1 << PHY_TMR_CSC_SHIFT_DIR_TRIG)
#define PHY_TMR_CSC_DIR_TRIG_FORCE1 (0x2 << PHY_TMR_CSC_SHIFT_DIR_TRIG)
#define PHY_TMR_CSC_DIR_TRIG_INVERT (0x3 << PHY_TMR_CSC_SHIFT_DIR_TRIG)

#define PHY_TMR_CSC_CMPE (0x1 << PHY_TMR_CSC_SHIFT_CMPE)

#endif /* __PHY_TIMER_H__ */
