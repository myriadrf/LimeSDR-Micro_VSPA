// =============================================================================
//! @file       vcpu.inc
//! @brief      VCPU assembly global definitions.
//! @author     NXP Semiconductors.
//! @copyright  Copyright (C) 2018 NXP Semiconductors.
// =============================================================================

#ifndef __VCPU_INC__
#define __VCPU_INC__

#if     (__AU_COUNT__ == 2)
#define NLANE2VEC8                  4
#define NLANE2VEC16                 3
#define NLANE2VEC32                 2
#define NLANE2VEC64                 1
#elif   (__AU_COUNT__ == 4)
#define NLANE2VEC8                  5
#define NLANE2VEC16                 4
#define NLANE2VEC32                 3
#define NLANE2VEC64                 2
#elif   (__AU_COUNT__ == 8)
#define NLANE2VEC8                  6
#define NLANE2VEC16                 5
#define NLANE2VEC32                 4
#define NLANE2VEC64                 3
#elif   (__AU_COUNT__ == 16)
#define NLANE2VEC8                  7
#define NLANE2VEC16                 6
#define NLANE2VEC32                 5
#define NLANE2VEC64                 4
#elif   (__AU_COUNT__ == 32)
#define NLANE2VEC8                  8
#define NLANE2VEC16                 7
#define NLANE2VEC32                 6
#define NLANE2VEC64                 5
#elif   (__AU_COUNT__ == 64)
#define NLANE2VEC8                  9
#define NLANE2VEC16                 8
#define NLANE2VEC32                 7
#define NLANE2VEC64                 6
#else
#error  "AU count not supported!"
#endif

#define _VSPA_VRA_REG_OFFSET(__n)   (__AU_COUNT__ * 4 * (__n))
#define _VR0                        _VSPA_VRA_REG_OFFSET(0)
#define _VR1                        _VSPA_VRA_REG_OFFSET(1)
#define _VR2                        _VSPA_VRA_REG_OFFSET(2)
#define _VR3                        _VSPA_VRA_REG_OFFSET(3)
#define _VR4                        _VSPA_VRA_REG_OFFSET(4)
#define _VR5                        _VSPA_VRA_REG_OFFSET(5)
#define _VR6                        _VSPA_VRA_REG_OFFSET(6)
#define _VR7                        _VSPA_VRA_REG_OFFSET(7)

#if (__AU_COUNT__ == 64) || (__AU_COUNT__ == 32)
#define _VSPA_VRAPTR_VAL(__n)       ((__n) & 0x1ff)
#define _VSPA_VRAPAGE_VAL(__n)      ((__n) >> 9)
#endif

#endif // __VCPU_INC__
