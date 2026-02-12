// SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
// Copyright 20172023 - 2025   NXP Semiconductors

// =============================================================================
//! @file       host.h
//! @brief      Host interface definitions.
//! @author     NXP Semiconductors.
// =============================================================================

#ifndef __HOST_H__
#define __HOST_H__

//! @ingroup    GROUP_VSPA_HOST
//! @{

// -----------------------------------------------------------------------------
// Host request interface:
// -----------------------------------------------------------------------------

//! @defgroup   GROUP_VSPA_HOST_EVENT   Host Event
//! @brief      Host request interface.
//! @{
//
//! @brief      Test if a host request is pending.
//! @retval     non-zero    if a host request is pending.
//! @retval     zero        if no host request is pending.
//!
//! This function returns non-zero if a host request is pending.
static inline uint32_t host_event(void) {
    return iord(CONTROL, 0x1 << 0); // Return CONTROL[0].
}

//! @brief      Clear a host request.
//! @return     This function does not return a value.
//!
//! This function clears a host request.
static inline void host_clear(void) {
    iowr(CONTROL, 0x1 << 0, 0x1 << 0); // Clear CONTROL[0].
}

//! @}

// -----------------------------------------------------------------------------
// Host mailbox interface:
// -----------------------------------------------------------------------------

//! @defgroup   GROUP_VSPA_HOST_MBOX    Host Mailbox
//! @brief      Host messaging interface.
//! @{

//! @brief      Enable Host to wake up VSPA when sending a message on mailbox 0.
//! @return     This function does not return a value.
//!
//! This function enables the host to wake up VSPA when sending a 32-bit
//! or 64-bit message on mailbox 0.
static inline void host_mbox0_enable(void) {
    iowr(CONTROL, 0x1 << 24, 0x1 << 24); // Set CONTROL[24].
}

//! @brief      Enable Host to wake up VSPA when sending a message on mailbox 1.
//! @return     This function does not return a value.
//!
//! This function enables the host to wake up VSPA when sending a 32-bit
//! or 64-bit message on mailbox 1.
static inline void host_mbox1_enable(void) {
    iowr(CONTROL, 0x1 << 25, 0x1 << 25); // Set CONTROL[25].
}

//! @brief      Enable Host to wake up VSPA when sending a message
//!             on either mailbox.
//! @return     This function does not return a value.
//!
//! This function enables the host to wake up VSPA when sending a 32-bit
//! or 64-bit message on either of the two mailboxes.
static inline void host_mbox_enable(void) { iowr(CONTROL, (0x1 << 25) | (0x1 << 24), (0x1 << 25) | (0x1 << 24)); }

//! @brief      Prevent Host from waking up VSPA when sending a message
//!             on mailbox 0.
//! @return     This function does not return a value.
//!
//! This function prevents the host from waking up VSPA when sending a 32-bit
//! or 64-bit message on mailbox 0.
static inline void host_mbox0_disable(void) {
    iowr(CONTROL, 0x1 << 24, 0x0 << 24); // Clear CONTROL[24].
}

//! @brief      Prevent Host from waking up VSPA when sending a message
//!             on mailbox 1.
//! @return     This function does not return a value.
//!
//! This function prevents the host from waking up VSPA when sending a 32-bit
//! or 64-bit message on mailbox 1.
static inline void host_mbox1_disable(void) {
    iowr(CONTROL, 0x1 << 25, 0x0 << 25); // Clear CONTROL[25].
}

//! @brief      Prevent Host from waking up VSPA when sending a message
//!             on either mailbox.
//! @return     This function does not return a value.
//!
//! This function prevents the host from waking up VSPA when sending a 32-bit
//! or 64-bit message on either of the two mailboxes.
static inline void host_mbox_disable(void) { iowr(CONTROL, (0x1 << 25) | (0x1 << 24), (0x0 << 25) | (0x0 << 24)); }

//! @brief      Test if a message is ready in mailbox 0.
//! @retval     non-zero    if a message is ready in mailbox 0.
//! @retval     zero        if no message is ready in mailbox 0.
//!
//! This function returns non-zero if a message is ready in mailbox 0.
static inline uint32_t host_mbox0_event(void) {
    return iord(CONTROL, 0x1 << 20); // Read CONTROL[20].
}

//! @brief      Test if a message is ready in mailbox 1.
//! @retval     non-zero    if a message is ready in mailbox 1.
//! @retval     zero        if no message is ready in mailbox 1.
//!
//! This function returns non-zero if a message is ready in mailbox 1.
static inline uint32_t host_mbox1_event(void) {
    return iord(CONTROL, 0x1 << 21); // Read CONTROL[21].
}

//! @brief      Read a 64-bit host message from mailbox 0.
//! @return     The 64-bit message.
//!
//! This function reads a 64-bit message from the host on mailbox 0.
static inline uint64_t host_mbox0_read(void) {
    uint64_t msg = iord64(VCPU_IN_0_MSB); // Read message.
    __fnop();                             // 1-cycle delay between read and clear.
    iowr(CONTROL, 0x1 << 20, 0x1 << 20);  // Clear event.
    return msg;                           // Return 64-bit message.
}

//! @brief      Read a 64-bit host message from mailbox 1.
//! @return     The 64-bit message.
//!
//! This function reads a 64-bit message from the host on mailbox 1.
static inline uint64_t host_mbox1_read(void) {
    uint64_t msg = iord64(VCPU_IN_1_MSB); // Read message.
    __fnop();                             // 1-cycle delay between read and clear.
    iowr(CONTROL, 0x1 << 21, 0x1 << 21);  // Clear event.
    return msg;                           // Return 64-bit message.
}

//! @brief      Returns non-zero if mailbox 0 already contains a message.
//! @retval     non-zero    VSPA cannot send a message on mailbox 0.
//! @retval     zero        VSPA can send a message on mailbox 0.
//!
//! This function returns non-zero if the outgoing mailbox 0 contains a message
//! from VSPA not yet read by the host.
static inline uint32_t host_mbox0_is_full(void) { return iord(VCPU_MBOX_STATUS, 0x1 << 0); }

//! @brief      Returns non-zero if input mailbox 0 contains a valid message
//! @retval     non-zero    There is a valid message in input mailbox 0
//! @retval     zero        There is a invalid message in input mailbox 0
//!
//! This function returns non-zero if the outgoing mailbox 0 contains a message valid for VSPA
static inline uint32_t vspa_mbox0_is_valid(void) { return iord(VCPU_MBOX_STATUS, 0x1 << 2); }

//! @brief      Returns non-zero if input mailbox 1 contains a valid message
//! @retval     non-zero    There is a valid message in input mailbox 1
//! @retval     zero        There is a invalid message in input mailbox 1
//!
//! This function returns non-zero if the outgoing mailbox 1 contains a message valid for VSPA
static inline uint32_t vspa_mbox1_is_valid(void) { return iord(VCPU_MBOX_STATUS, 0x1 << 3); }

//! @brief      Returns non-zero if mailbox 1 already contains a message.
//! @retval     non-zero    VSPA cannot send a message on mailbox 1.
//! @retval     zero        VSPA can send a message on mailbox 1.
//!
//! This function returns non-zero if the outgoing mailbox 1 contains a message
//! from VSPA not yet read by the host.
static inline uint32_t host_mbox1_is_full(void) { return iord(VCPU_MBOX_STATUS, 0x1 << 1); }

//! @brief      Send a 64-bit message to the host via mailbox 0.
//! @param      msg     The 64-bit message to send.
//! @return     This function does not return a value.
//!
//! This function sends a 64-bit message to the host on mailbox 0.
//!
//! @note       This function does not check if the mailbox is already full
//!             before sending the message. If the mailbox is already full,
//!             the host may not read the message that is already in the mailbox.
//!             Use host_mbox0_is_full() to check if mailbox 0 is already full.
static inline void host_mbox0_post(uint64_t msg) { iowr64(VCPU_OUT_0_MSB, msg); }

//! @brief      Send a 64-bit message to the host via mailbox 1.
//! @param      msg     The 64-bit message to send.
//! @return     This function does not return a value.
//!
//! This function sends a 64-bit message to the host on mailbox 1.
//!
//! @note       This function does not check if the mailbox is already full
//!             before sending the message. If the mailbox is already full,
//!             the host may not read the message that is already in the mailbox.
//!             Use host_mbox1_is_full() to check if mailbox 1 is already full.
static inline void host_mbox1_post(uint64_t msg) { iowr64(VCPU_OUT_1_MSB, msg); }

static inline void host_mbox0_clear(void) { iowr(CONTROL, 0x1 << 20, 0x1 << 20); }

static inline void host_mbox1_clear(void) { iowr(CONTROL, 0x1 << 21, 0x1 << 21); }

static inline void host_mbox_clear(void) { iowr(CONTROL, (0x1 << 21) | (0x1 << 20), (0x1 << 21) | (0x1 << 20)); }

//! @}

// -----------------------------------------------------------------------------
// Host flags interface:
// -----------------------------------------------------------------------------

//! @defgroup   GROPU_VSPA_HOST_FLAG    Host Flags
//! @brief      Host flags interface.
//! @{

//! @brief      Test if a host vspa flags go is pending.
//! @retval     non-zero    if a host vspa flags go is pending.
//! @retval     zero        if no host vspa flags go is pending.
//!
//! This function returns non-zero if a host vspa flags go is pending.
static inline uint32_t host_flags_go(void) { return iord(CONTROL, 0x1 << 6); }

//! @brief    Allow Host to wake up VSPA through hardware flags.
//! @return   This function does not return a value.
//!
//! This function enables the Host to wake up VSPA when the Host sets a hardware
//! flag.
static inline void host_flags_enable(void) { iowr(CONTROL, (0x1 << 12) | (0x1 << 11), (0x1 << 12) | (0x1 << 11)); }

//! @brief    Allow Host to wake up VSPA through lower hardware flags.
//! @return   This function does not return a value.
//!
//! This function enables the Host to wake up VSPA when the Host sets a lower
//! hardware flag.
static inline void host_flags_enable_lo(void) { iowr(CONTROL, (0x1 << 11), (0x1 << 11)); }

//! @brief    Allow Host to wake up VSPA through upper hardware flags.
//! @return   This function does not return a value.
//!
//! This function enables the Host to wake up VSPA when the Host sets a upper
//! hardware flag.
static inline void host_flags_enable_hi(void) { iowr(CONTROL, (0x1 << 12), (0x1 << 12)); }

//! @brief    Prevent Host from waking up VSPA through hardware flags.
//! @return   This function does not return a value.
//!
//! This function disables the Host to wake up VSPA when the Host sets
//! a hardware flag.
static inline void host_flags_disable(void) { iowr(CONTROL, (0x1 << 12) | (0x1 << 11), (0x0 << 12) | (0x0 << 11)); }

#pragma cplusplus on

//! @brief      Returns non-zero if any of the Host-to-VSPA flags is set.
//! @retval     non-zero    At least one Host flag is set.
//! @retval     zero        No host flag is set.
//!
//! This function returns non-zero if at least one Host flag is set,
//! it returns zero if no Host flag is set.
static inline uint64_t host_flags_event(void) { return MAKEDWORD(iord(HOST_VCPU_FLAGS1), iord(HOST_VCPU_FLAGS0)); }

//! @brief      Returns non-zero if at least one Host-to-VSPA flag in select
//!             Host-to-VSPA flags is set.
//! @param      mask    Bit mask specifying Host flags to test.
//!                     Bits set to 1 correspond to flags to test. Bits
//!                     set to 0 correspond to flags to ignore.
//! @retval     non-zero    At least one Host flag is set.
//! @retval     zero        No host flag is set.
//!
//! This function returns non-zero if at least one of the Host flags specified
//! with @a mask is set, it returns zero if none of the Host flags is set.
static inline uint64_t host_flags_event(uint64_t mask) {
    return MAKEDWORD(iord(HOST_VCPU_FLAGS1, HIWORD(mask)), iord(HOST_VCPU_FLAGS0, LOWORD(mask)));
}

//! @brief      Returns non-zero if at least one Host-to-VSPA flag in select
//!             Host-to-VSPA flags0 is set.
//! @param      mask    Bit mask specifying Host flags to test.
//!                     Bits set to 1 correspond to flags to test. Bits
//!                     set to 0 correspond to flags to ignore.
//! @retval     non-zero    At least one Host flag is set.
//! @retval     zero        No host flag is set.
//!
//! This function returns non-zero if at least one of the Host flags specified
//! with @a mask is set, it returns zero if none of the Host flags is set.
static inline uint32_t host_flags_event_lo(uint32_t mask) { return iord(HOST_VCPU_FLAGS0, mask); }

//! @brief      Returns non-zero if at least one Host-to-VSPA flag in select
//!             Host-to-VSPA flags1 is set.
//! @param      mask    Bit mask specifying Host flags to test.
//!                     Bits set to 1 correspond to flags to test. Bits
//!                     set to 0 correspond to flags to ignore.
//! @retval     non-zero    At least one Host flag is set.
//! @retval     zero        No host flag is set.
//!
//! This function returns non-zero if at least one of the Host flags specified
//! with @a mask is set, it returns zero if none of the Host flags is set.
static inline uint32_t host_flags_event_hi(uint32_t mask) { return iord(HOST_VCPU_FLAGS1, mask); }

//! @brief      Clear all Host-to-VSPA flags.
//! @return     This function does not return a value.
//!
//! This function clears all Host flags.
static inline void host_flags_clear(void) {
    iowr(HOST_VCPU_FLAGS0, 0xFFFFFFFF);
    iowr(HOST_VCPU_FLAGS1, 0xFFFFFFFF);
}

//! @brief      Clear specific Host-to-VSPA flags.
//! @param      mask    Bit mask specifying Host flags to clear.
//!                     Bits set to 1 correspond to flags to clear. Bits
//!                     set to 0 correspond to flags that are left untouched.
//! @return     This function does not return a value.
//!
//! This function clears the Host flags specified by the bit mask @a mask.
static inline void host_flags_clear(uint64_t mask) {
    iowr(HOST_VCPU_FLAGS0, LOWORD(mask));
    iowr(HOST_VCPU_FLAGS1, HIWORD(mask));
}

//! @brief      Clear specific Host-to-VSPA flags0.
//! @param      mask    Bit mask specifying Host flags to clear.
//!                     Bits set to 1 correspond to flags to clear. Bits
//!                     set to 0 correspond to flags that are left untouched.
//! @return     This function does not return a value.
//!
//! This function clears the Host flags specified by the bit mask @a mask.
static inline void host_flags_clear_lo(uint32_t mask) { iowr(HOST_VCPU_FLAGS0, mask); }

//! @brief      Clear specific Host-to-VSPA flags1.
//! @param      mask    Bit mask specifying Host flags to clear.
//!                     Bits set to 1 correspond to flags to clear. Bits
//!                     set to 0 correspond to flags that are left untouched.
//! @return     This function does not return a value.
//!
//! This function clears the Host flags specified by the bit mask @a mask.
static inline void host_flags_clear_hi(uint32_t mask) { iowr(HOST_VCPU_FLAGS1, mask); }

//! @brief      Set all VSPA-to-Host flags.
//! @return     This function does not return a value.
//!
//! This function sets all VSPA-to-Host flags.
static inline void vcpu_flags_raise(void) {
    iowr(VCPU_HOST_FLAGS0, 0xFFFFFFFF);
    iowr(VCPU_HOST_FLAGS1, 0xFFFFFFFF);
}

//! @brief      Set select VSPA-to-Host flags.
//! @param      mask    Bit mask specifying the flags to set.
//!                     Bits set to 1 correspond to flags to set. Bits set to 0
//!                     correspond to flags that are left untouched.
//! @return     This function does not return a value.
//!
//! This function sets the flags specified by the bit mask @a mask.
static inline void vcpu_flags_raise(uint64_t mask) {
    iowr(VCPU_HOST_FLAGS0, LOWORD(mask));
    iowr(VCPU_HOST_FLAGS1, HIWORD(mask));
}

//! @brief      Set all the bits of VSPA-to-Host flag0 register.
//! @return     This function does not return a value.
//!
//! This function set all the bits of VSPA-to-Host flag0 register
static inline void vcpu_flags_raise_lo(uint32_t mask) { iowr(VCPU_HOST_FLAGS0, mask); }

//! @brief      Set all the bits of VSPA-to-Host flag1 register.
//! @return     This function does not return a value.
//!
//! This function set all the bits of VSPA-to-Host flag1 register
static inline void vcpu_flags_raise_hi(uint32_t mask) { iowr(VCPU_HOST_FLAGS1, mask); }

//! @brief      Returns non-zero if any of the VSPA-to-Host flags is set.
//! @retval     non-zero    At least one VCPU flag is set.
//! @retval     zero        No VSPA flag is set.
//!
//! This function returns non-zero if at least one VCPU flag is set,
//! it returns zero if no VSPA flag is set.
static inline uint64_t vcpu_flags(void) { return MAKEDWORD(iord(VCPU_HOST_FLAGS1), iord(VCPU_HOST_FLAGS0)); }

//! @brief      Returns non-zero if at least one VSPA-to-Host flag in select
//!             VSPA-to-Host flags is set.
//! @param      mask    Bit mask specifying VCPU flags to test.
//!                     Bits set to 1 correspond to flags to test. Bits
//!                     set to 0 correspond to flags to ignore.
//! @retval     non-zero    At least one VSPA flag is set.
//! @retval     zero        No VSPA flag is set.
//!
//! This function returns non-zero if at least one of the VCPU flags specified
//! with @a mask is set, it returns zero if none of the VCPU flags is set.
static inline uint64_t vcpu_flags(uint64_t mask) {
    return MAKEDWORD(iord(VCPU_HOST_FLAGS1, HIWORD(mask)), iord(VCPU_HOST_FLAGS0, LOWORD(mask)));
}

//! @brief      Returns non-zero if any bit of the VSPA-to-Host flag0 is set
//! @retval     non-zero    At least one VCPU flag is set.
//! @retval     zero        No VCPU flag is set.
//! This function returns non-zero if at least one of the VCPU flags is set in
//! VCPU to host flag0, it returns zero if none of the VCPU flags is set.
static inline uint32_t vcpu_flags_lo(void) { return iord(VCPU_HOST_FLAGS0); }

//! @brief      Returns non-zero if any bit of the VSPA-to-Host flag0 is set
//! @param      mask    Bit mask specifying VCPU flags to test.
//!                     Bits set to 1 correspond to flags to test. Bits
//!                     set to 0 correspond to flags to ignore.
//! @retval     non-zero    At least one VCPU flag is set.
//! @retval     zero        No VCPU flag is set.
//! This function returns non-zero if at least one of the VCPU flags specified
//! with @a mask is set, it returns zero if none of the VCPU flags is set.
static inline uint32_t vcpu_flags_lo(uint32_t mask) { return iord(VCPU_HOST_FLAGS0, mask); }

//! @brief      Returns non-zero if any bit of the VSPA-to-Host flag1 is set
//! @retval     non-zero    At least one VCPU flag is set.
//! @retval     zero        No VCPU flag is set.
//! This function returns non-zero if at least one of the VCPU flags is set in
//! VCPU to host flag1, it returns zero if none of the VCPU flags is set.
static inline uint32_t vcpu_flags_hi(void) { return iord(VCPU_HOST_FLAGS1); }

//! @brief      Returns non-zero if any bit of the VSPA-to-Host flag1 is set
//! @param      mask    Bit mask specifying VCPU flags to test.
//!                     Bits set to 1 correspond to flags to test. Bits
//!                     set to 0 correspond to flags to ignore.
//! @retval     non-zero    At least one VCPU flag is set.
//! @retval     zero        No VCPU flag is set.
//! This function returns non-zero if at least one of the VCPU flags specified
//! with @a mask is set, it returns zero if none of the VCPU flags is set.
static inline uint32_t vcpu_flags_hi(uint32_t mask) { return iord(VCPU_HOST_FLAGS1, mask); }

#pragma cplusplus reset

//! @}

// -----------------------------------------------------------------------------
// Control functions:
// -----------------------------------------------------------------------------

//! @brief      Reset the host interface.
//! @return     This function does not return a value.
//!
//! This function clears any pending host request, empties the host mailboxes
//! and clears any pending mailbox event.
static inline void host_reset(void) {
    host_clear();
    host_flags_clear();
    host_mbox0_read();
    host_mbox1_read();
    host_mbox_clear();
}

//! @}
#endif // __HOST_H__
