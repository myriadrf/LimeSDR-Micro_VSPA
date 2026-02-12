// SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
// Copyright 2017 - 2025   NXP Semiconductors

// =============================================================================
//! @file       ext.h
//! @brief      External event definitions.
//! @author     NXP Semiconductors.
// =============================================================================

#ifndef __EXT_H__
#define __EXT_H__

//! @ingroup    GROUP_VSPA_EXT
//! @{

// -----------------------------------------------------------------------------
// Macros:
// -----------------------------------------------------------------------------

//! @brief      Transform an external event channel into a bit mask.
//! @param      chan    The external event channel.
//! @return     The bit mask corresponding to the external event channel.
#define EXT_MASK(chan) (1 << (index))

// -----------------------------------------------------------------------------
// Functions:
// -----------------------------------------------------------------------------

#pragma cplusplus on

//! @defgroup   GROUP_VSPA_HOST_EVENT   External Go Event

//! @brief      Disable all external events.
//! @return     This function does not return a value.
//!
//! This function disables all external events.
static inline void ext_disable(void) { iowr(EXT_GO_ENA, 0x0); }

//! @brief      Disable select external events.
//! @param      mask    Bit mask specifying external events to disable.
//!                     Bits set to 1 correspond to events to disable. Bits
//!                     set to 0 correspond to events that are left untouched.
//! @return     This function does not return a value.
//!
//! This function disables the external events specified byt the bit mask
//! @a mask.
static inline void ext_disable(uint32_t mask) { iowr(EXT_GO_ENA, mask, ~mask); }

//! @brief      Enable all external events.
//! @return     This function does not return a value.
//!
//! This function enables all external events.
static inline void ext_enable(void) { iowr(EXT_GO_ENA, 0xFFFFFFFF); }

//! @brief      Enable select external events.
//! @param      mask    Bit mask specifying external events to enable.
//!                     Bits set to 1 correspond to events to enable. Bits
//!                     set to 0 correspond to events that are left untouched.
//! @return     This function does not return a value.
//!
//! This function enables the external events specified by the bit mask
//! @a mask.
static inline void ext_enable(uint32_t mask) { iowr(EXT_GO_ENA, mask, mask); }

//! @brief      Returns non-zero if at least one external event is enabled.
//! @retval     non-zero    At least one external event is enabled.
//! @retval     zero        No external event is enabled.
//!
//! This function returns 0 if no external event is enabled. It returns non-zero
//! if at least 1 external event is enabled.
static inline uint32_t ext_is_enabled(void) { return iord(EXT_GO_ENA); }

//! @brief      Returns non-zero if at least one external event in select
//!             external events is enabled.
//! @param      mask        Bit mask specifying external events to check.
//! @retval     non-zero    At least one external event is enabled.
//! @retval     zero        No external event is enabled.
//!
//! This function returns 0 if no external event, among the events specified
//! by the bit mask @a mask, is enabled. It returns non-zero if at least 1 event
//! is enabled.
static inline uint32_t ext_is_enabled(uint32_t mask) { return iord(EXT_GO_ENA, mask); }

//! @brief      Returns non-zero if at least one external event in select
//!             external go events request is pending.
//! @param      mask        Bit mask specifying external go events to check.
//! @retval     non-zero    At least one external go event is pending.
//! @retval     zero        No external go event is pending.
//!
//! This function returns 0 if no external event, among the events specified
//! by the bit mask @a mask, is pending. It returns non-zero if at least 1 event
//! is pending.

static inline uint32_t ext_is_pending(uint32_t mask) { return iord(EXT_GO_STAT, mask); }

static inline uint32_t ext_is_pending(void) { return iord(EXT_GO_STAT); }

static inline uint32_t ext_event(void) { return (ext_is_pending() & ext_is_enabled()); }

static inline uint32_t ext_event(uint32_t mask) { return (ext_is_pending(mask) & ext_is_enabled(mask)); }

//! @brief      Clear all external events.
//! @return     This function does not return a value.
//!
//! This function clears all external events.
static inline void ext_clear(void) { iowr(EXT_GO_STAT, 0xFFFFFFFF); }

//! @brief      Clear select external events.
//! @param      mask    Bit mask specifying external events to clear.
//!                     Bits set to 1 correspond to events to clear. Bits
//!                     set to 0 correspond to events that are left untouched.
//! @return     This function does not return a value.
//!
//! This function clears the external events specified by the bit mask @a mask.
static inline void ext_clear(uint32_t mask) { iowr(EXT_GO_STAT, mask); }

//! @brief      Reset the external event interface.
//! @return     This function does not return a value.
//!
//! This function disables all external events and clears any pending external
//! event.
static inline void ext_reset(void) {
    ext_disable();
    ext_clear();
}

#pragma cplusplus reset

//! @}
#endif // __EXT_H__
