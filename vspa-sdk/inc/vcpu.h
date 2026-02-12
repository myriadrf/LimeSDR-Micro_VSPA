// SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
// Copyright 2017 - 2025   NXP Semiconductors

// =============================================================================
//! @file       vcpu.h
//! @brief      Vector central processing unit definitions.
//! @author     NXP Semiconductors.
// =============================================================================

#ifndef __VCPU_H__
#define __VCPU_H__

//! @addtogroup GROUP_VCPU
//! @{

// -----------------------------------------------------------------------------
//! @defgroup   GROUP_VCPU_TYPES    Fundamental Data Types
//! @{
// -----------------------------------------------------------------------------

#if !defined(__ASSEMBLER__)
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <vspa/intrinsics.h>
#include "vspa.h"
#endif

#include "iohw.h"

#if !defined(__ASSEMBLER__)
typedef __fx16 fixed16_t; //!< 16-bit fixed point data type.
typedef __fp16 float16_t; //!< 16-bit floating point data type.
typedef float float32_t;  //!< 32-bit floating point data type.
typedef double float64_t; //!< 64-bit floating point data type.

//! @brief      16-bit complex fixed point data type.
typedef struct cfixed16_t {
    fixed16_t real; //!< Real part.
    fixed16_t imag; //!< Imaginary part.
} cfixed16_t;

//! @brief      16-bit complex floating point data type.
typedef struct cfloat16_t {
    float16_t real; //!< Real part.
    float16_t imag; //!< Imaginary part.
} cfloat16_t;

//! @brief      32-bit complex floating point data type.
typedef vspa_complex_float32 cfloat32_t;

//! @brief      64-bit complex floating point data type.
typedef vspa_complex_float64 cfloat64_t;

//! @}

// -----------------------------------------------------------------------------
//! @defgroup   GROUP_VCPU_RUNTIME  Runtime Functions
//! @{
// -----------------------------------------------------------------------------

//! @brief      VSPA program entry point function pointer type.
typedef void (*entry_t)(void) __noreturn;

#if defined(__LA1575__)

//! @brief      Current VSPA program entry point.
extern entry_t __entry;

#pragma cplusplus on

//! @brief      Set the VSPA program entry point.
//! @param      fn  Specifies the address of the VSPA program entry point.
//! @return     This function does not return a value.
//!
//! This function sets the VSPA program entry point to the address
//! of the function specified with @a fn.
static inline void entry(entry_t fn) { __entry = fn; }

//! @brief      Returns the current VSPA program entry point.
//! @return     The current VSPA program entry point.
//!
//! This function returns the current VSPA program entry point.
static inline entry_t entry(void) { return __entry; }

#pragma cplusplus reset

#else // __LA1785__

#pragma cplusplus on

//! @brief      Set the VSPA program entry point.
//! @param      fn  Specifies the address of the VSPA program entry point.
//! @return     This function does not return a value.
//!
//! This function sets the VSPA program entry point to the address
//! of the function specified with @a fn.
static inline void entry(entry_t fn) { iowr(VCPU_GO_ADDR, (uint32_t)fn); }

//! @brief      Returns the current VSPA program entry point.
//! @return     The current VSPA program entry point.
//!
//! This function returns the current VSPA program entry point.
static inline entry_t entry(void) { return (entry_t)iord(VCPU_GO_ADDR); }

//! @brief      Define the VSPA stack base DMEM address
//! @param      base: DMEM base address for stack
//! @return     This function does not return a value.
//!
//! This function sets the VSPA stack base address. The SP register will be
//! set to this value every time VCPU wakes up from idle to process a GO event.
static inline void stack_base(void *base) { iowr(VCPU_GO_STACK, (uint32_t)base); }

//! @brief      Returns the VSPA stack base DMEM address
//! @return     The VSPA stack base address.
//!
//! This function returns the VSPA stack base DMEM address.
static inline uint32_t stack_base(void) { return iord(VCPU_GO_STACK); }

#pragma cplusplus reset

#endif // __LA1785__

//! @brief      VSPA core initialization function.
//! @return     This function does not return a value.
//!
//! This function initializes the VSPA core and peripherals, typically after
//! the VSPA image is initially loaded.
//!
//! @note       Chip-specific initializations should be done in init().
//!             For example, the exact number of GPIO registers is defined
//!             at chip level, so any GPIO initialization cannot be done
//!             with __init() and must be done inside init().
extern void __init(void) __noreturn;

//! @brief      VSPA core termination function.
//! @return     This function never returns.
//!
//! This function disables all events that can wake up VSPA
//! and clears any pending event. It then puts the core in idle mode.
extern void __exit(void) __noreturn;

//! @brief      User-defined VSPA C program main function.
//! @return     This function never returns.
//!
//! This function implements the VSPA application.
extern void main(void) __noreturn;

//! @brief      User-defined chip-level application initialization function.
//! @return     This function does not return a value.
//!
//! This function is implemented by the user to initialize off-core peripherals,
//! interfaces, and any software module required by the application.
extern void init(void);

//! @brief      Boot procedure to get VSPA started after Host loads VSPA image.
//! @return     This function does not return a value.
//!
//! This function implements the mailbox exchange required with the Host
//! after VSPA image is loaded and code execution started.
extern void boot(void);

//! @brief      User-defined chip-level application termination function.
//! @return     This function never returns.
//!
//! This function is implemented by the user to terminate the application:
//! it does any application cleanup if necessary and signals the termination
//! to the Host.
//!
//! @attention  This function must end by invoking __exit() to terminate
//!             code execution on VSPA core.
extern void exit(int status) __noreturn;

//! @brief      Enter low power mode.
//! @return     This function does not return a value.
//!
//! This function stops code execution until an event occurs.
//!
static inline void __idle(void) __noreturn { __builtin_done(); }

//! @}

// -----------------------------------------------------------------------------
//! @defgroup   GROUP_VCPU_CRT  C Runtime Functions
//! @{
// -----------------------------------------------------------------------------

// Single precision floating point arithmetic ----------------------------------

//! @brief      Single precision floating point real addition.
//! @param      x   Single precision floating point real number.
//! @param      y   Single precision floating point real number.
//! @return     The single precision floating point result of x+y.
//!
//! This function implements a single precision floating point real addition
//! operation.
extern float __f_add(float x, float y);

//! @brief      Single precision floating point real division.
//! @param      x   Single precision floating point real number.
//! @param      y   Single precision floating point real number.
//! @return     The single precision floating point result of x/y.
//!
//! This function implements a single precision floating point real division
//! operation.
extern float __f_div(float x, float y);

//! @brief      Single precision real floating point multiplication.
//! @param      x   Single precision floating point real number.
//! @param      y   Single precision floating point real number.
//! @return     The single precision floating point result of x*y.
//!
//! This function implements a single precision floating point real
//! multiplication operation.
extern float __f_mul(float x, float y);

//! @brief      Single precision real floating point subtraction.
//! @param      x   Single precision floating point real number.
//! @param      y   Single precision floating point real number.
//! @return     The single precision floating point result of x-y.
//!
//! This function implements a single precision floating point real subtraction
//! operation.
extern float __f_sub(float x, float y);

//! @brief      Single precision floating point real absolute value.
//! @param      x   Single precision floating point real number.
//! @return     The single precision floating point absolute value of @a x.
//!
//! This function implements a single precision floating point absolute value
//! operation.
extern float fabsf(float x);

//! @brief      Single precision floating point real square root value.
//! @param      x   Single precision floating point real number.
//! @return     The single precision floating point square root value of @a x.
//!
//! This function implements a single precision floating point square root
//! operation.
extern float sqrtf(float x);

//! @brief      Single precision floating point real reciprocal value.
//! @param      x   Single precision floating point real number.
//! @return     The single precision floating point reciprocal value of @a x.
//!
//! This function implements a single precision floating point reciprocal
//! operation.
extern float rcpf(float x);

//! @brief      Single precision floating point real reciprocal square root
//!             value.
//! @param      x   Single precision floating point real number.
//! @return     The single precision floating point reciprocal square root value
//!             of @a x.
//!
//! This function implements a single precision floating point reciprocal square
//! root operation.
extern float rsqrtf(float x);

//! @}

// -----------------------------------------------------------------------------
//! @defgroup   GROUP_VCPU_STL  C Standard Library Functions
//! @{
// -----------------------------------------------------------------------------

// string.h --------------------------------------------------------------------

extern void memset8(void *p, int c, size_t n);
extern void memset16(void *p, int c, size_t n);
extern void memset32(void *p, int c, size_t n);
extern void memsetvec(void *p, int c, size_t n);

extern void memset16circ(void *p, int c, size_t n, void *b, size_t s);
extern void memset32circ(void *p, int c, size_t n, void *b, size_t s);
extern void memsetveccirc(void *p, int c, size_t n, void *b, size_t s);

extern void memclr(void *p, size_t n);
extern void memclr16(void *p, size_t n);
extern void memclr32(void *p, size_t n);
extern void memclrvec(void *p, size_t n);

extern void memclrcirc(void *p, size_t n, void *b, size_t s);
extern void memclr16circ(void *p, size_t n, void *b, size_t s);
extern void memclr32circ(void *p, size_t n, void *b, size_t s);
extern void memclrveccirc(void *p, size_t n, void *b, size_t s);

/*! @todo extern void memcpy16(void *restrict d, void const *restrict s, size_t n); */
/*! @todo extern void memcpy32(void *restrict d, void const *restrict s, size_t n); */

//! @brief      Copy a vector-aligned array to a vector-aligned array.
//! @param      d   Destination address.
//! @param      s   Source address.
//! @param      n   Number of vectors to copy.
//! @return     This function does not return a value.
//! @attention  Destination and source arrays cannot overlap.
extern void veccpy(void *restrict d, void const *restrict s, size_t n);

//! @brief      Copy an array to a vector-aligned array.
//! @param      d   Destination address.
//! @param      s   Source address.
//! @param      n   Number of vectors to copy.
//! @return     This function does not return a value.
//! @attention  Destination and source arrays cannot overlap.
extern void memcpy32_w2v(void *restrict d, void const *restrict s, size_t n);

//! @brief      Copy a circular array to a vector-aligned array.
//! @param      d   Destination address.
//! @param      s   Source address (circular buffer).
//! @param      n   Number of vectors to copy.
//! @param      b   Source base address (circular buffer).
//! @param      nc  Number of addressable units in circular buffer
//! @attention  Destination and source arrays cannot overlap.
extern void memcpy32_circw2v(void *restrict d, void const *restrict s, size_t n, void *restrict b, size_t nc);

//! @brief      Copy a vector-aligned array to a 32-bit aligned array.
//! @param      d   Destination address.
//! @param      s   Source address.
//! @param      n   Number of 16-bit data to copy.
//! @return     This function does not return a value.
//! @attention  Destination and source arrays cannot overlap.
extern void memcpy32_v2w(void *restrict d, void const *restrict s, size_t n);

//! @brief      Copy 32bit aligned data into another 32bit aligned array
//! @param      d   Destination address.
//! @param      s   Source address;
//! @param      n   Number 32bit values to copy.
extern void memcpy32_w2w(void *d, void *s, size_t n);

//! @brief      Copy a vector-aligned array to a 32-bit aligned array.
//! @param      d   Destination address.
//! @param      s   Source address.
//! @param      n   Number of 32-bit data to copy.
//! @return     This function does not return a value.
//! @attention  Destination and source arrays cannot overlap.
extern void veccpy32(void *restrict d, void const *restrict s, size_t n);

//! @brief      Copy a vector-aligned array to 16-bit aligned array.
//! @param      d   Destination address.
//! @param      s   Source address.
//! @param      n   Number of 16-bit data to copy.
//! @return     This function does not return a value.
//! @attention  Destination and source arrays cannot overlap.
extern void veccpy16(void *restrict d, void const *restrict s, size_t n);

/*! @todo extern void memcpy16circ(void *restrict d, void const *restrict s, size_t n,
    void *bd, size_t sd, void const *bs, size_t ss); */
/*! @todo extern void memcpy32circ(void *restrict d, void const *restrict s, size_t n,
    void *bd, size_t sd, void const *bs, size_t ss); */
/*! @todo extern void memcpyveccirc(void *restrict d, void const *restrict s, size_t n,
    void *bd, size_t sd, void const *bs, size_t ss); */

// errno.h ---------------------------------------------------------------------

//! @brief      Error code.
//!
//! This variable represents the last error code.
extern int errno;

#define EAXIQ 0x00001000
#define EAXIS 0x00002000
#define EDMAC 0x00003000
#define EEXT 0x00004000
#define EGPIO 0x00005000
#define EHOST 0x00006000
#define EIPPU 0x00007000
#define EVCPU 0x00008000
#define EUSER 0x00100000

#define EAXIQBANK (EAXIQ + 0x001)
#define EAXIQFIFO (EAXIQ + 0x002)

#define EAXISEVT (EAXIS + 0x001)

#define EDMACXFR (EDMAC + 0x001)
#define EDMACCFG (EDMAC + 0x002)
#define EDMACEVT (EDMAC + 0x003)

#define EEXTEVT (EEXT + 0x001)

#define EEXTEVT (EEXT + 0x001)

#define EIPPUBUSY (EIPPU + 0x001)
#define EIPPUFULL (EIPPU + 0x002)

// -----------------------------------------------------------------------------
//! @brief      Modulo increment operation on an unsigned 32-bit integer
//! @param      base Unsigned 32-bit number.
//! @param      incr Increment.
//! @param      size Modulo size.
//! @return     modulo incremented base.
// -----------------------------------------------------------------------------
static inline uint32_t __inc_mod(uint32_t base, int incr, size_t size) {
    base += incr;
    base -= base >= size ? size : 0;
    return base;
}

#if 0 /* defined in CW_VSPA_v10.3.x\VSPA\VSPA_Tools\include\cstdlib */
// stdlib.h --------------------------------------------------------------------

//! @brief      Return an integer absolute value.
//! @param      x   The signed integer value.
//! @return     The absolute value of @a x.
//!
//! Ths function abs() returns the absolute value of its integer operand
//! @a x.
//!
//! @warning    The result of abs(INT_MIN) is INT_MAX.
static inline unsigned int abs(int x)
{
    return __abs(x);
}
#endif

//! @}

// -----------------------------------------------------------------------------
//! @defgroup   GROUP_VCPU_MISC     Miscellaneous Utilities
//! @{
// -----------------------------------------------------------------------------

//! @brief      Delay processing by a number of cycles.
//! @param      delay   Number of cycles.
//! @return     This function does not return a value.
//!
//! This function performs a delay of @a delay cycles.
extern void delay_cycle(int delay);

//! @brief      Busy-wait on a logical condition.
//! @param      Logical condition.
//! @return     This macro does not return a value.
//!
//! This macro implements a busy-wait loop waiting for a logical condition
//! to become true.
#define WAIT(cond)         \
    do {                   \
        do {               \
        } while (!(cond)); \
    } while (0)

//! @brief      Compile-time cast macro.
//! @param      type    Data type to cast to.
//! @param      var     Variable to cast.
//! @return     This macro returns the variable with a modified data type
//!             @a type.
//!
//! This macro changes the type of a variable at compile-time
//! (i.e. no format conversion occurs at run-time).
#define CAST(type, var) *((type *)&var)

#pragma gcc_extensions on

//! @brief      Minimum value between to real numbers.
//! @param      a   Real number.
//! @param      b   Real number.
//! @return     The minimum real number between @a a and @a b.
//!
//! This macro returns the minimum value between two real numbers,
//! with type-safety checking and without run-time side-effects.
#define MIN(a, b)               \
    ({                          \
        __typeof__(a) _a = (a); \
        __typeof__(b) _b = (b); \
        (void)(&_a == &_b);     \
        _a < _b ? _a : _b;      \
    })

//! @brief      Maximum value between to real numbers.
//! @param      a   Real number.
//! @param      b   Real number.
//! @return     The maximum real number between @a a and @a b.
//!
//! This macro returns the minimum value between two real numbers,
//! with type-safety checking and without run-time side-effects.
#define MAX(a, b)               \
    ({                          \
        __typeof__(a) _a = (a); \
        __typeof__(b) _b = (b); \
        (void)(&_a == &_b);     \
        _a > _b ? _a : _b;      \
    })

#pragma gcc_extensions reset

#pragma cplusplus on

#if 0 // TBD TECH: CW Build Id:200817 have added these to built-in intrinsics?
//! @brief      Minimum value between two real unsigned numbers.
//! @param      a   Real unsigned number.
//! @param      b   Real unsigned number.
//! @return     The minimum real unsigned number between @a a and @a b.
//! @cycle      1.
static inline uint32_t __min(uint32_t a, uint32_t b)
{
    uint32_t m;
    __asm__ volatile ("min %0, %1, %2": "=g"(m): "g"(a), "g"(b));
    return m;
}

//! @brief      Minimum value between two real signed numbers.
//! @param      a   Real signed number.
//! @param      b   Real signed number.
//! @return     The minimum real signed number between @a a and @a b.
//! @cycle      1.
static inline int __min(int a, int b)
{
    int m;
    __asm__ volatile ("min %0, %1, %2": "=g"(m): "g"(a), "g"(b));
    return m;
}

//! @brief      Maximum value between two real unsigned numbers.
//! @param      a   Real unsigned number.
//! @param      b   Real unsigned number.
//! @return     The maximum real unsigned number between @a a and @a b.
//! @cycle      1.
static inline uint32_t __max(uint32_t a, uint32_t b)
{
    uint32_t m;
    __asm__ volatile ("max %0, %1, %2": "=g"(m): "g"(a), "g"(b));
    return m;
}

//! @brief      Maximum value between two real signed numbers.
//! @param      a   Real signed number.
//! @param      b   Real signed number.
//! @return     The maximum real signed number between @a a and @a b.
//! @cycle      1.
static inline int __max(int a, int b)
{
    int m;
    __asm__ volatile ("max %0, %1, %2": "=g"(m): "g"(a), "g"(b));
    return m;
}
#endif

#pragma cplusplus reset

//! @brief      Ceiled division.
//! @param      x   the dividend.
//! @param      y   the divisor.
//! @return     The ceiled quotient: ceil(x/y).
#define DIV_CEIL(x, y) (((x) + (y)-1) / (y))

//! @brief      Rounded division.
//! @param      x   the dividend.
//! @param      y   the divisor.
//! @return     The approximate rounded quotient: round(x/y).
#define DIV_ROUND(x, y) (((x) + (y) / 2) / (y))

//! @brief      Round up to multiple of 64.
//! @param      x   number to be rounded up.
//! @return     Rounded-Up to multiple of 64.
#define ROUNDUP64(x) (((x) + 64 - 1) & (0xFFFFFFC0))

#if !defined(__VSPA3__)
//! @brief      Count number of 1-bit in a 32-bit word.
//! @param      word    the 32-bit word.
//! @return     The number of 1-bit in @a word.
//! @see        Hacker's Delight, figure 5-2.
static inline int __cnto(uint32_t word) {
    word = word - ((word >> 1) & 0x55555555);
    word = (word & 0x33333333) + ((word >> 2) & 0x33333333);
    word = (word + (word >> 4)) & 0x0F0F0F0F;
    word = word + (word >> 8);
    word = word + (word >> 16);
    return word & 0x0000003F;
}
#else

//! @brief      Count number of 1-bit in a 32-bit word.
//! @param      word    the 32-bit word.
//! @return     The number of 1-bit in @a word.
//! @cycle      1.
static inline int __cnto(uint32_t word) {
    int n;
    __asm__ volatile("cnto %0, %1" : "=g"(n) : "g"(word));
    return n;
}
#endif

//! @brief      Count number of 0-bit in a 32-bit word.
//! @param      word    the 32-bit word.
//! @return     The number of 0-bit in @a word.
//! @cycle      1.
static inline int __cntz(uint32_t word) {
    int n;
    __asm__ volatile("cntz %0, %1" : "=g"(n) : "g"(word));
    return n;
}

//! @brief      Count number of leading (strating from MSB) zeros
//!             in a 32-bit word.
//! @param      word    the 32-bit word.
//! @return     The number of leading zeros in @a word.
//! @cycle      1.
static inline int __cntl(uint32_t word) {
    if (word == 0) {
        return 32;
    }
    int n;
    __asm__ volatile("cntl %0, %1" : "=g"(n) : "g"(word));
    return n;
}

//! @brief      Count trailing zeros in a 16-bit word.
//! @param      word    the 16-bit word.
//! @return     The number of trailing zeros.
static inline int ctz16(uint16_t hword) {
    uint32_t result;
    uint32_t word = ((uint32_t)hword) | (1 << 16);
    __ff1(result, word ^ (word - 1));
    return result;
}

//! @brief      Count trailing zeros in a 32-bit word.
//! @param      word    the 32-bit word.
//! @return     The number of trailing zeros.
static inline int ctz32(uint32_t word) {
    if (word == 0) {
        return 32;
    }
    uint32_t result;
    __ff1(result, word ^ (word - 1));
    return result;
}

//! @brief      Extract the least significant byte from a half-word.
//! @param      hw  the half-word.
//! @return     The least significant byte from @a hw.
#define LOBYTE(hw) ((hw)&0x00FF)

//! @brief      Extract the most significant byte from a half-word.
//! @param      hw  the half-word.
//! @return     The most significant byte from @a hw.
#define HIBYTE(hw) ((hw) >> 8)

//! @brief      Extract the least significant half-word from a word.
//! @param      w   the word.
//! @return     The least significant half-word from @a w.
#define LOHWORD(w) ((w)&0x0000FFFF)

//! @brief      Extract the most significant half-word from a word.
//! @param      w   the word.
//! @return     The most significant half-word from @a w.
#define HIHWORD(w) ((w) >> 16)

//! @brief      Extract the least significant word from a double-word.
//! @param      dw  the double-word.
//! @return     The least significant word from @a dw.
#define LOWORD(dw) ((dw)&0x00000000FFFFFFFFULL)

//! @brief      Extract the most significant word from a double-word.
//! @param      dw  the double-word.
//! @return     The most significant word from @a dw.
#define HIWORD(dw) ((dw) >> 32)

//! @brief      Make a half-word from 2 bytes.
//! @param      hi  the most significant byte.
//! @param      lo  the least significant byte.
//! @return     The half-word made of @a hi and @a lo.
#define MAKEHWORD(hi, lo) ((((uint16_t)(hi)) << 8) | ((uint16_t)(lo)))

//! @brief      Make a word from 2 half-words.
//! @param      hi  the most significant half-word.
//! @param      lo  the least significant half-word.
//! @return     The word made of @a hi and @a lo.
#define MAKEWORD(hi, lo) ((((uint32_t)(hi)) << 16) | ((uint32_t)(lo)))

//! @brief      Make a double-word from 2 words.
//! @param      hi  the most significant word.
//! @param      lo  the least significant word.
//! @return     The double-word made of @a hi and @a lo.
#define MAKEDWORD(hi, lo) ((((uint64_t)(hi)) << 32) | ((uint64_t)(lo)))

//! @brief      Get the target core identifier.
//! @return     The core identifier.
//! @see        vspa_atid_t
static inline uint32_t atid(void) { return iord(PARAM0, 0x7F); }

//! @brief      Get the VCPU data memory size.
//! @return     The data memory size in number of minimum addressable unit.
//! @note       This function returns a number of half-words on VSPA2,
//!             a number of bytes on VSPA3.
static inline uint32_t vcpu_dmem_size(void) {
#if defined(__VSPA2__)
    return (iord(VCPU_DMEM_BYTES) >> 1);
#elif defined(__VSPA3__)
    return iord(VCPU_DMEM_BYTES);
#endif
}

//! @brief      Read the VCPU hardware version number.
//! @return     The VCPU hardware version number.
static inline uint32_t vcpu_hwver(void) { return iord(HWVERSION); }

#pragma cplusplus on
//! @brief      Read the VCPU software version number.
//! @return     The VCPU sofwtare version number.
static inline uint32_t vcpu_swver(void) { return iord(SWVERSION); }

//! @brief      Write the VCPU software version number.
//! @param      swver   The software version number to write.
//! @return     This function does not return a value.
static inline void vcpu_swver(uint32_t swver) { iowr(SWVERSION, swver); }
#pragma cplusplus reset

//! @}

#define CGU_SHIFT_VSPA 0
#define CGU_SHIFT_DMAC 1
#define CGU_SHIFT_BKPT 2
#define CGU_SHIFT_DRI 4
#define CGU_SHIFT_IPPU 5
#define CGU_SHIFT_DBG 6
#define CGU_SHIFT_FECU 7
#define CGU_SHIFT_DMAC_AUX 8

#define CGU_FIELD_VSPA (0x1 << CGU_SHIFT_VSPA)
#define CGU_FIELD_DMAC (0x1 << CGU_SHIFT_DMAC)
#define CGU_FIELD_BKPT (0x1 << CGU_SHIFT_BKPT)
#define CGU_FIELD_DRI (0x1 << CGU_SHIFT_DRI)
#define CGU_FIELD_IPPU (0x1 << CGU_SHIFT_IPPU)
#define CGU_FIELD_DBG (0x1 << CGU_SHIFT_DBG)
#define CGU_FIELD_FECU (0x1 << CGU_SHIFT_FECU)
#define CGU_FIELD_DMAC_AUX (0x1 << CGU_SHIFT_DMAC_AUX)

#pragma cplusplus on
//! @brief      Disable clock gating.
//! @return     This function does not return a value.
//! @note       Clock gating is enabled by default after reset.
static inline void cgu_disable(void) { iowr(0x24 >> 2, CGU_FIELD_VSPA, CGU_FIELD_VSPA); }

//! @brief      Enable clock gating.
//! @return     This function does not return a value.
//! @note       Clock gating is enabled by default after reset.
static inline void cgu_enable(void) { iowr(0x24 >> 2, CGU_FIELD_VSPA, 0); }

//! @brief      Disable clock gating.
//! @param      mask    Bit mask specifying which block(s) do not use clock gating.
//! @return     This function does not return a value.
static inline void cgu_disable(uint32_t mask) {
    iowr(0x24 >> 2, mask, mask); // Set the bit to enable the clock always.
}

//! @brief      Enable clock gating.
//! @param      mask    Bit mask specifying which block(s) use clock gating.
//! @return     This function does not return a value.
static inline void cgu_enable(uint32_t mask) {
    iowr(0x24 >> 2, mask, 0); // Clear the bit to disable the clock when block is not used.
}
#pragma cplusplus reset

static inline int nco2int(int freq) {
    if (freq < 0) {
        return -(((uint32_t)freq) ^ 0xFFFFFFFF);
    } else {
        return freq;
    }
}

static inline int int2nco(int freq) {
    if (freq < 0) {
        return (int)(((uint32_t)__abs(freq)) ^ 0xFFFFFFFF);
    } else {
        return freq;
    }
}

/**
 * @brief       Cast a 32-bit unsigned integer to a single precision
 *              floating point number.
 * @param       u32 The 32-bit unsigned integer.
 * @return      The same value interpreted as a single precision floating point
 *              number.
 */
static inline float __cast_float32(uint32_t u32) { return *((float *)&u32); }

#pragma cplusplus on
/**
 * @brief       Cast a single precision floating point number to a 32-bit
 *              unsigned integer.
 * @param       f32 The single precision floating point number.
 * @return      The same value interpreted as unsigned integer.
 */
static inline uint32_t __cast_uint32(float f32) { return *((uint32_t *)&f32); }

/**
 * @brief       Cast a complex half precision floating point number to a 32-bit
 *              unsigned integer.
 * @param       f32 The complex half precision floating point number.
 * @return      The same value interpreted as unsigned integer.
 */
static inline uint32_t __cast_uint32(cfloat16_t cf16) { return *((uint32_t *)&cf16); }

#pragma cplusplus reset

/**
 * @brief       Cast a single precision floating point number to a 32-bit
 *              signed integer.
 * @param       f32 The single precision floating point number.
 * @return      The same value interpreted as signed integer.
 */
static inline int32_t __cast_int32(float f32) { return *((int32_t *)&f32); }

/**
 * @brief       Cast a 32-bit unsigned integer to a complex half precision
 *              floating point number.
 * @param       u32 The 32-bit unsigned integer.
 * @return      The same value interpreted as a complex half precision
 *              floating point number.
 */
static inline cfloat16_t __cast_cfloat16(uint32_t u32) { return *((cfloat16_t *)&u32); }

static inline fixed16_t __cast_fixed16(uint32_t u32) { return *((fixed16_t *)&u32); }

//! @brief      inlined function which just returns.
//! @return     This function does not return a value.
extern void rts_only(void);

//! @}
#endif // __ASSEMBLER__
#endif // __VCPU_H__
