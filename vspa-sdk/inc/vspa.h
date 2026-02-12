// SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
// Copyright 20172024 - 2025   NXP Semiconductors

// =============================================================================
//! @file       vspa.h
//! @brief      Vector signal processing acceleration definitions.
//! @author     NXP Semiconductors
// =============================================================================

#ifndef __VSPA_H__
#define __VSPA_H__

//! @addtogroup GROUP_VSPA
//! @{

#define __align(x) __attribute__((aligned(x)))
#define __aligned(x) __attribute__((aligned(x)))
#define __section(x) __attribute__((section(x)))
#define __noreturn __attribute((noreturn))
#define __noinline __attribute((noinline))
#define __weak __attribute((weak))
#define __optimize(x) __attribute__((optimize(x)))
#define ASM(...) __asm volatile(__VA_ARGS__)

#define SIZEVEC8 (8 * __AU_COUNT__)
#define SIZEVEC16 (4 * __AU_COUNT__)
#define SIZEVEC32 (2 * __AU_COUNT__)
#define SIZEVEC64 (1 * __AU_COUNT__)

// _VR  = # of halfwords of one  VRA register
// _VRH = # of halfwords of half VRA register
#define _VR (__AU_COUNT__ * 4)
#define _VRH (__AU_COUNT__ * 2)

#define ROUNDUP32TOVEC(x) (((x) + SIZEVEC32 - 1) / SIZEVEC32)

#if defined(__VSPA2__)
#define ADDR8(x) (((uint32_t) & (x)) << 1)
#define ADDR16(x) (((uint32_t) & (x)) << 0)
#define ADDR32(x) (((uint32_t) & (x)) >> 1)
#define SIZEOF8(x) (sizeof(x) * 2)
#define SIZEOF16(x) (sizeof(x) * 1)
#define SIZEOF32(x) ((sizeof(x) + 1) / 2)
#define SIZEOF64(x) ((sizeof(x) + 3) / 4)
#define SIZEOFVEC(x) ((sizeof(x) + SIZEVEC16 - 1) / SIZEVEC16)
#define SIZEMAU 1
#define __align_vec __align(SIZEVEC16)
#elif defined(__VSPA3__)
#define ADDR8(x) (((uint32_t) & (x)) << 0)
#define ADDR16(x) (((uint32_t) & (x)) >> 1)
#define ADDR32(x) (((uint32_t) & (x)) >> 2)
#define SIZEOF8(x) (sizeof(x) * 1)
#define SIZEOF16(x) ((sizeof(x) + 1) / 2)
#define SIZEOF32(x) ((sizeof(x) + 3) / 4)
#define SIZEOF64(x) ((sizeof(x) + 7) / 8)
#define SIZEOFVEC(x) ((sizeof(x) + SIZEVEC8 - 1) / SIZEVEC8)
#define SIZEMAU 2
#define __align_vec __align(SIZEVEC8)
#else
#error "Architecture not supported!"
#endif

#define NLANE8 (8 * __AU_COUNT__)  //!< Number of  8-bit lanes per vector.
#define NLANE16 (4 * __AU_COUNT__) //!< Number of 16-bit lanes per vector.
#define NLANE32 (2 * __AU_COUNT__) //!< Number of 32-bit lanes per vector.
#define NLANE64 (1 * __AU_COUNT__) //!< Number of 64-bit lanes per vector.

#define NVEC8(x) (((x) + (NLANE8 - 1)) / NLANE8)    //!< Number of vectors required to store x  8-bit data.
#define NVEC16(x) (((x) + (NLANE16 - 1)) / NLANE16) //!< Number of vectors required to store x 16-bit data.
#define NVEC32(x) (((x) + (NLANE32 - 1)) / NLANE32) //!< Number of vectors required to store x 32-bit data.
#define NVEC64(x) (((x) + (NLANE64 - 1)) / NLANE64) //!< Number of vectors required to store x 64-bit data.

#define NVECPAIRS32(x) ((NVEC32(x) + 1) / 2) //!< Number of vector pairs required to store x 32-bit data.
#define NVECQUADS32(x) ((NVEC32(x) + 3) / 4) //!< Number of vector quad lines required to store x 32-bit data.

//! @brief      Number of vectors required to store an array.
//! @param      expr    Expression, typically the array variable name.
//! @return     The minimum number of vectors required to store the array.
#define NVEC(expr) NVEC8(SIZEOF8(expr))

//! @brief      Number of lanes of width sizeof(type) in a vector.
//! @param      type    Data type (or expression) specifying the lane width.
//! @return     The number of lanes of width sizeof(type) in a vector.
#define NLANE(type) (NLANE8 / SIZEOF8(type))

#define UPHW SIZEMAU
#define __vec_aligned __align_vec

#define __nop() ASM("nop .asmvol")
#define __fnop() ASM("fnop .asmvol")
#define __bkpt()     \
    do {             \
        __nop();     \
        __swbreak(); \
        __nop();     \
    } while (0)
#if defined(__DEBUG__)
#define DBGTRAP() __bkpt()
#define ASSERT(cond)   \
    do {               \
        if (!(cond)) { \
            __bkpt();  \
        }              \
    } while (0)
#else // __DEBUG__
#define DBGTRAP()
#define ASSERT(cond)
#endif // __DEBUG__
#define CASSERT(cond, msg) __static_assert(cond, msg)

#define __trap() \
    do {         \
    } while (1)

//! @}

#endif // __VSPA_H__
