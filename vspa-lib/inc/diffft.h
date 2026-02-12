// SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
// Copyright 2016 - 2025   NXP Semiconductors

// ===========================================================================
//! @file            diffft.h
//! @brief           DIF FFT library interface definitions.
//! @ingroup         GROUP_FFT
//!
//! The diffft.h header defines the DIF FFT library application programming
//! interface.
//!
// ===========================================================================

#ifndef __DIFFFT_H__
#define __DIFFFT_H__

#include <stddef.h>

// -----------------------------------------------------------------------------
//! @defgroup		GROUP_FFT FFT Library
//! @brief          FFT function library
//!
//! This library contains function prototypes for radix 2 FFTs
//! The FFT Library provides the following functions:
//!   - 64 pt DIF FFT HFX_SFL:
//!      - fftDIF64_hfx_sfl(): 16-bit fixed point in, 32-bit floating point out 64pt DIF FFT
//!   - 64 pt DIF IFFT HFX_SFL:
//!      - ifftDIF64_hfx_sfl(): 16-bit fixed point in, 32-bit floating point out 64pt DIF IFFT
//!   - 128 pt DIF FFT HFX_SFL:
//!      - fftDIF128_hfx_sfl(): 16-bit fixed point in, 32-bit floating point out 128pt DIF FFT
//!   - 128 pt DIF IFFT HFX_SFL:
//!      - ifftDIF128_hfx_sfl(): 16-bit fixed point in, 32-bit floating point out 128pt DIF IFFT
//!   - 256 pt DIF FFT HFX_SFL:
//!      - fftDIF256_hfx_sfl(): 16-bit fixed point in, 32-bit floating point out 256pt DIF FFT
//!   - 256 pt DIF IFFT HFX_SFL:
//!      - ifftDIF256_hfx_sfl(): 16-bit fixed point in, 32-bit floating point out 256pt DIF IFFT
//!   - 512 pt DIF FFT HFX_SFL:
//!      - fftDIF512_hfx_sfl(): 16-bit fixed point in, 32-bit floating point out 512pt DIF FFT
//!   - 512 pt DIF IFFT HFX_SFL:
//!      - ifftDIF512_hfx_sfl(): 16-bit fixed point in, 32-bit floating point out 512pt DIF IFFT
//!   - 1024 pt DIF FFT HFX_SFL:
//!      - fftDIF1024_hfx_sfl(): 16-bit fixed point in, 32-bit floating point out 1024pt DIF FFT
//!   - 1024 pt DIF IFFT HFX_SFL:
//!      - ifftDIF1024_hfx_sfl(): 16-bit fixed point in, 32-bit floating point out 1024pt DIF IFFT
//!   - 2048 pt DIF FFT HFX_SFL:
//!      - fftDIF2048_hfx_sfl(): 16-bit fixed point in, 32-bit floating point out 2048pt DIF FFT
//!   - 2048 pt DIF IFFT HFX_SFL:
//!      - ifftDIF2048_hfx_sfl(): 16-bit fixed point in, 32-bit floating point out 2048pt DIF IFFT
//!   - 64 pt DIF FFT HFX_HFX:
//!      - fftDIF64_hfx_hfx(): 16-bit fixed point in, 16-bit fixed point out 64pt DIF FFT
//!   - 64 pt DIF IFFT HFX_HFX:
//!      - ifftDIF64_hfx_hfx(): 16-bit fixed point in, 16-bit fixed point out 64pt DIF IFFT
//!   - 128 pt DIF FFT HFX_HFX:
//!      - fftDIF128_hfx_hfx(): 16-bit fixed point in, 16-bit fixed point out 128pt DIF FFT
//!   - 128 pt DIF IFFT HFX_HFX:
//!      - ifftDIF128_hfx_hfx(): 16-bit fixed point in, 16-bit fixed point out 128pt DIF IFFT
//!   - 256 pt DIF FFT HFX_HFX:
//!      - fftDIF256_hfx_hfx(): 16-bit fixed point in, 16-bit fixed point out 256pt DIF FFT
//!   - 256 pt DIF IFFT HFX_HFX:
//!      - ifftDIF256_hfx_hfx(): 16-bit fixed point in, 16-bit fixed point out 256pt DIF IFFT
//!   - 512 pt DIF FFT HFX_HFX:
//!      - fftDIF512_hfx_hfx(): 16-bit fixed point in, 16-bit fixed point out 512pt DIF FFT
//!   - 512 pt DIF IFFT HFX_HFX:
//!      - ifftDIF512_hfx_hfx(): 16-bit fixed point in, 16-bit fixed point out 512pt DIF IFFT
//!   - 1024 pt DIF FFT HFX_HFX:
//!      - fftDIF1024_hfx_hfx(): 16-bit fixed point in, 16-bit fixed point out 1024pt DIF FFT
//!   - 1024 pt DIF IFFT HFX_HFX:
//!      - ifftDIF1024_hfx_hfx(): 16-bit fixed point in, 16-bit fixed point out 1024pt DIF IFFT
//!   - 2048 pt DIF FFT HFX_HFX:
//!      - fftDIF2048_hfx_hfx(): 16-bit fixed point in, 16-bit fixed point out 2048pt DIF FFT
//!   - 2048 pt DIF IFFT HFX_HFX:
//!      - ifftDIF2048_hfx_hfx(): 16-bit fixed point in, 16-bit fixed point out 2048pt DIF IFFT
//!   - 64 pt DIF FFT HFX_HFL:
//!      - fftDIF64_hfx_hfl(): 16-bit fixed point in, 16-bit floating point out 64pt DIF FFT
//!   - 64 pt DIF IFFT HFX_HFL:
//!      - ifftDIF64_hfx_hfl(): 16-bit fixed point in, 16-bit floating point out 64pt DIF IFFT
//!   - 128 pt DIF FFT HFX_HFL:
//!      - fftDIF128_hfx_hfl(): 16-bit fixed point in, 16-bit floating point out 128pt DIF FFT
//!   - 128 pt DIF IFFT HFX_HFL:
//!      - ifftDIF128_hfx_hfl(): 16-bit fixed point in, 16-bit floating point out 128pt DIF IFFT
//!   - 256 pt DIF FFT HFX_HFL:
//!      - fftDIF256_hfx_hfl(): 16-bit fixed point in, 16-bit floating point out 256pt DIF FFT
//!   - 256 pt DIF IFFT HFX_HFL:
//!      - ifftDIF256_hfx_hfl(): 16-bit fixed point in, 16-bit floating point out 256pt DIF IFFT
//!   - 512 pt DIF FFT HFX_HFL:
//!      - fftDIF512_hfx_hfl(): 16-bit fixed point in, 16-bit floating point out 512pt DIF FFT
//!   - 512 pt DIF IFFT HFX_HFL:
//!      - ifftDIF512_hfx_hfl(): 16-bit fixed point in, 16-bit floating point out 512pt DIF IFFT
//!   - 1024 pt DIF FFT HFX_HFL:
//!      - fftDIF1024_hfx_hfl(): 16-bit fixed point in, 16-bit floating point out 1024pt DIF FFT
//!   - 1024 pt DIF IFFT HFX_HFL:
//!      - ifftDIF1024_hfx_hfl(): 16-bit fixed point in, 16-bit floating point out 1024pt DIF IFFT
//!   - 2048 pt DIF FFT HFX_HFL:
//!      - fftDIF2048_hfx_hfl(): 16-bit fixed point in, 16-bit floating point out 2048pt DIF FFT
//!   - 2048 pt DIF IFFT HFX_HFL:
//!      - ifftDIF2048_hfx_hfl(): 16-bit fixed point in, 16-bit floating point out 2048pt DIF IFFT
//!
//! @{
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// variable
// -----------------------------------------------------------------------------

// ---------------------------------------------------------------------------
//! @brief           64 pt SP FFT using decimation in frequency approach for 16 bit fixed point input data, 32 bit floating point
//! output data
//!
//! @param[in]       pIn   		Input buffer address
//! @param[out]      pOut  		Output buffer address
//! @param[in]       pBuff 		Base of circular input buffer
//! @param[in]       cbuffSize  Size of circular buffer in half-word units
//! @return          Void.
//! @cycle
//! @stack         	 0
//!
//! This function calculates 64 pt FFT using decimation in frequency (DIF approach)
//! Equivalent MATLAB command: y = fft(x)
//!
//! @attention       Inputs must be in linear order
//! @attention       Outputs are in bit-reversed order
//! @attention       The output buffer must be vector-aligned.
// ---------------------------------------------------------------------------
extern void fftDIF64_hfx_sfl(vspa_complex_fixed16 const *pIn, // Input buffer pointer for holding half fixed point complex values
                             vspa_complex_float32 *pOut, // Output buffer pointer for holding single floating point complex values.
                             vspa_complex_fixed16 const *pBuff, // Base of circular buffer
                             unsigned int const cbuffSize       // Size of circular buffer in half-words
);

// ---------------------------------------------------------------------------
//! @brief           64 pt SP IFFT using decimation in frequency approach for 16 bit fixed point input data, 32 bit floating point
//! output data
//!
//! @param[in]       pIn   		Input buffer address
//! @param[out]      pOut  		Output buffer address
//! @param[in]       pBuff 		Base of circular input buffer
//! @param[in]       cbuffSize  Size of circular buffer in half-word units
//! @return          Void.
//! @cycle
//! @stack         	 0
//!
//! This function calculates 64 pt IFFT using decimation in frequency (DIF approach)
//! Equivalent MATLAB command: y = N*ifft(x)
//!
//! @attention       Inputs must be in linear order
//! @attention       Outputs are in bit-reversed order
//! @attention       The output buffer must be vector-aligned.
// ---------------------------------------------------------------------------
extern void ifftDIF64_hfx_sfl(vspa_complex_fixed16 const *pIn, // Input buffer pointer for holding half fixed point complex values
                              vspa_complex_float32 *pOut, // Output buffer pointer for holding single floating point complex values.
                              vspa_complex_fixed16 const *pBuff, // Base of circular buffer
                              unsigned int const cbuffSize       // Size of circular buffer in half-words
);

// ---------------------------------------------------------------------------
//! @brief           128 pt SP FFT using decimation in frequency approach for 16 bit fixed point input data, 32 bit floating point
//! output data
//!
//! @param[in]       pIn   		Input buffer address
//! @param[out]      pOut  		Output buffer address
//! @param[in]       pBuff 		Base of circular input buffer
//! @param[in]       cbuffSize  Size of circular buffer in half-word units
//! @return          Void.
//! @cycle
//! @stack         	 0
//!
//! This function calculates 128 pt FFT using decimation in frequency (DIF approach)
//! Equivalent MATLAB command: y = fft(x)
//!
//! @attention       Inputs must be in linear order
//! @attention       Outputs are in bit-reversed order
//! @attention       The output buffer must be vector-aligned.
// ---------------------------------------------------------------------------
extern void fftDIF128_hfx_sfl(vspa_complex_fixed16 const *pIn, // Input buffer pointer for holding half fixed point complex values
                              vspa_complex_float32 *pOut, // Output buffer pointer for holding single floating point complex values.
                              vspa_complex_fixed16 const *pBuff, // Base of circular buffer
                              unsigned int const cbuffSize       // Size of circular buffer in half-words
);

// ---------------------------------------------------------------------------
//! @brief           128 pt SP IFFT using decimation in frequency approach for 16 bit fixed point input data, 32 bit floating point
//! output data
//!
//! @param[in]       pIn   		Input buffer address
//! @param[out]      pOut  		Output buffer address
//! @param[in]       pBuff 		Base of circular input buffer
//! @param[in]       cbuffSize  Size of circular buffer in half-word units
//! @return          Void.
//! @cycle
//! @stack         	 0
//!
//! This function calculates 128 pt IFFT using decimation in frequency (DIF approach)
//! Equivalent MATLAB command: y = N*ifft(x)
//!
//! @attention       Inputs must be in linear order
//! @attention       Outputs are in bit-reversed order
//! @attention       The output buffer must be vector-aligned.
// ---------------------------------------------------------------------------
extern void
ifftDIF128_hfx_sfl(vspa_complex_fixed16 const *pIn,   // Input buffer pointer for holding half fixed point complex values
                   vspa_complex_float32 *pOut,        // Output buffer pointer for holding single floating point complex values.
                   vspa_complex_fixed16 const *pBuff, // Base of circular buffer
                   unsigned int const cbuffSize       // Size of circular buffer in half-words
);

// ---------------------------------------------------------------------------
//! @brief           256 pt SP FFT using decimation in frequency approach for 16 bit fixed point input data, 32 bit floating point
//! output data
//!
//! @param[in]       pIn   		Input buffer address
//! @param[out]      pOut  		Output buffer address
//! @param[in]       pBuff 		Base of circular input buffer
//! @param[in]       cbuffSize  Size of circular buffer in half-word units
//! @return          Void.
//! @cycle
//! @stack         	 0
//!
//! This function calculates 256 pt FFT using decimation in frequency (DIF approach)
//! Equivalent MATLAB command: y = fft(x)
//!
//! @attention       Inputs must be in linear order
//! @attention       Outputs are in bit-reversed order
//! @attention       The output buffer must be vector-aligned.
// ---------------------------------------------------------------------------
extern void fftDIF256_hfx_sfl(vspa_complex_fixed16 const *pIn, // Input buffer pointer for holding half fixed point complex values
                              vspa_complex_float32 *pOut, // Output buffer pointer for holding single floating point complex values.
                              vspa_complex_fixed16 const *pBuff, // Base of circular buffer
                              unsigned int const cbuffSize       // Size of circular buffer in half-words
);

// ---------------------------------------------------------------------------
//! @brief           256 pt SP IFFT using decimation in frequency approach for 16 bit fixed point input data, 32 bit floating point
//! output data
//!
//! @param[in]       pIn   		Input buffer address
//! @param[out]      pOut  		Output buffer address
//! @param[in]       pBuff 		Base of circular input buffer
//! @param[in]       cbuffSize  Size of circular buffer in half-word units
//! @return          Void.
//! @cycle
//! @stack         	 0
//!
//! This function calculates 256 pt IFFT using decimation in frequency (DIF approach)
//! Equivalent MATLAB command: y = N*ifft(x)
//!
//! @attention       Inputs must be in linear order
//! @attention       Outputs are in bit-reversed order
//! @attention       The output buffer must be vector-aligned.
// ---------------------------------------------------------------------------
extern void
ifftDIF256_hfx_sfl(vspa_complex_fixed16 const *pIn,   // Input buffer pointer for holding half fixed point complex values
                   vspa_complex_float32 *pOut,        // Output buffer pointer for holding single floating point complex values.
                   vspa_complex_fixed16 const *pBuff, // Base of circular buffer
                   unsigned int const cbuffSize       // Size of circular buffer in half-words
);

// ---------------------------------------------------------------------------
//! @brief           512 pt SP FFT using decimation in frequency approach for 16 bit fixed point input data, 32 bit floating point
//! output data
//!
//! @param[in]       pIn   		Input buffer address
//! @param[out]      pOut  		Output buffer address
//! @param[in]       pBuff 		Base of circular input buffer
//! @param[in]       cbuffSize  Size of circular buffer in half-word units
//! @return          Void.
//! @cycle
//! @stack         	 0
//!
//! This function calculates 512 pt FFT using decimation in frequency (DIF approach)
//! Equivalent MATLAB command: y = fft(x)
//!
//! @attention       Inputs must be in linear order
//! @attention       Outputs are in bit-reversed order
//! @attention       The output buffer must be vector-aligned.
// ---------------------------------------------------------------------------
extern void fftDIF512_hfx_sfl(vspa_complex_fixed16 const *pIn, // Input buffer pointer for holding half fixed point complex values
                              vspa_complex_float32 *pOut, // Output buffer pointer for holding single floating point complex values.
                              vspa_complex_fixed16 const *pBuff, // Base of circular buffer
                              unsigned int const cbuffSize       // Size of circular buffer in half-words
);

// ---------------------------------------------------------------------------
//! @brief           512 pt SP IFFT using decimation in frequency approach for 16 bit fixed point input data, 32 bit floating point
//! output data
//!
//! @param[in]       pIn   		Input buffer address
//! @param[out]      pOut  		Output buffer address
//! @param[in]       pBuff 		Base of circular input buffer
//! @param[in]       cbuffSize  Size of circular buffer in half-word units
//! @return          Void.
//! @cycle
//! @stack         	 0
//!
//! This function calculates 512 pt IFFT using decimation in frequency (DIF approach)
//! Equivalent MATLAB command: y = N*ifft(x)
//!
//! @attention       Inputs must be in linear order
//! @attention       Outputs are in bit-reversed order
//! @attention       The output buffer must be vector-aligned.
// ---------------------------------------------------------------------------
extern void
ifftDIF512_hfx_sfl(vspa_complex_fixed16 const *pIn,   // Input buffer pointer for holding half fixed point complex values
                   vspa_complex_float32 *pOut,        // Output buffer pointer for holding single floating point complex values.
                   vspa_complex_fixed16 const *pBuff, // Base of circular buffer
                   unsigned int const cbuffSize       // Size of circular buffer in half-words
);

// ---------------------------------------------------------------------------
//! @brief           1024 pt SP FFT using decimation in frequency approach for 16 bit fixed point input data, 32 bit floating point
//! output data
//!
//! @param[in]       pIn   		Input buffer address
//! @param[out]      pOut  		Output buffer address
//! @param[in]       pBuff 		Base of circular input buffer
//! @param[in]       cbuffSize  Size of circular buffer in half-word units
//! @return          Void.
//! @cycle
//! @stack         	 0
//!
//! This function calculates 1024 pt FFT using decimation in frequency (DIF approach)
//! Equivalent MATLAB command: y = fft(x)
//!
//! @attention       Inputs must be in linear order
//! @attention       Outputs are in bit-reversed order
//! @attention       The output buffer must be vector-aligned.
// ---------------------------------------------------------------------------
extern void
fftDIF1024_hfx_sfl(vspa_complex_fixed16 const *pIn,   // Input buffer pointer for holding half fixed point complex values
                   vspa_complex_float32 *pOut,        // Output buffer pointer for holding single floating point complex values.
                   vspa_complex_fixed16 const *pBuff, // Base of circular buffer
                   unsigned int const cbuffSize       // Size of circular buffer in half-words
);

// ---------------------------------------------------------------------------
//! @brief           1024 pt SP IFFT using decimation in frequency approach for 16 bit fixed point input data, 32 bit floating point
//! output data
//!
//! @param[in]       pIn   		Input buffer address
//! @param[out]      pOut  		Output buffer address
//! @param[in]       pBuff 		Base of circular input buffer
//! @param[in]       cbuffSize  Size of circular buffer in half-word units
//! @return          Void.
//! @cycle
//! @stack         	 0
//!
//! This function calculates 1024 pt IFFT using decimation in frequency (DIF approach)
//! Equivalent MATLAB command: y = N*ifft(x)
//!
//! @attention       Inputs must be in linear order
//! @attention       Outputs are in bit-reversed order
//! @attention       The output buffer must be vector-aligned.
// ---------------------------------------------------------------------------
extern void
ifftDIF1024_hfx_sfl(vspa_complex_fixed16 const *pIn,   // Input buffer pointer for holding half fixed point complex values
                    vspa_complex_float32 *pOut,        // Output buffer pointer for holding single floating point complex values.
                    vspa_complex_fixed16 const *pBuff, // Base of circular buffer
                    unsigned int const cbuffSize       // Size of circular buffer in half-words
);

// ---------------------------------------------------------------------------
//! @brief           2048 pt SP FFT using decimation in frequency approach for 16 bit fixed point input data, 32 bit floating point
//! output data
//!
//! @param[in]       pIn   		Input buffer address
//! @param[out]      pOut  		Output buffer address
//! @param[in]       pBuff 		Base of circular input buffer
//! @param[in]       cbuffSize  Size of circular buffer in half-word units
//! @return          Void.
//! @cycle
//! @stack         	 0
//!
//! This function calculates 2048 pt FFT using decimation in frequency (DIF approach)
//! Equivalent MATLAB command: y = fft(x)
//!
//! @attention       Inputs must be in linear order
//! @attention       Outputs are in bit-reversed order
//! @attention       The output buffer must be vector-aligned.
// ---------------------------------------------------------------------------
extern void
fftDIF2048_hfx_sfl(vspa_complex_fixed16 const *pIn,   // Input buffer pointer for holding half fixed point complex values
                   vspa_complex_float32 *pOut,        // Output buffer pointer for holding single floating point complex values.
                   vspa_complex_fixed16 const *pBuff, // Base of circular buffer
                   unsigned int const cbuffSize       // Size of circular buffer in half-words
);

// ---------------------------------------------------------------------------
//! @brief           2048 pt SP IFFT using decimation in frequency approach for 16 bit fixed point input data, 32 bit floating point
//! output data
//!
//! @param[in]       pIn   		Input buffer address
//! @param[out]      pOut  		Output buffer address
//! @param[in]       pBuff 		Base of circular input buffer
//! @param[in]       cbuffSize  Size of circular buffer in half-word units
//! @return          Void.
//! @cycle
//! @stack         	 0
//!
//! This function calculates 2048 pt IFFT using decimation in frequency (DIF approach)
//! Equivalent MATLAB command: y = N*ifft(x)
//!
//! @attention       Inputs must be in linear order
//! @attention       Outputs are in bit-reversed order
//! @attention       The output buffer must be vector-aligned.
// ---------------------------------------------------------------------------
extern void
ifftDIF2048_hfx_sfl(vspa_complex_fixed16 const *pIn,   // Input buffer pointer for holding half fixed point complex values
                    vspa_complex_float32 *pOut,        // Output buffer pointer for holding single floating point complex values.
                    vspa_complex_fixed16 const *pBuff, // Base of circular buffer
                    unsigned int const cbuffSize       // Size of circular buffer in half-words
);

// ---------------------------------------------------------------------------
//! @brief           64 pt HFX FFT using decimation in frequency approach for 16 bit fixed point input data, 16 bit fixed point
//! output data
//!
//! @param[in]       pIn   		Input buffer address
//! @param[out]      pOut  		Output buffer address
//! @param[in]       pBuff 		Base of circular input buffer
//! @param[in]       cbuffSize  Size of circular buffer in half-word units
//! @return          Void.
//! @cycle
//! @stack         	 0
//!
//! This function calculates 64 pt FFT using decimation in frequency (DIF approach)
//! Equivalent MATLAB command: y = (1/N)*fft(x)
//!
//! @attention       Inputs must be in linear order
//! @attention       Outputs are in bit-reversed order
//! @attention       The output buffer must be vector-aligned.
// ---------------------------------------------------------------------------
extern void fftDIF64_hfx_hfx(vspa_complex_fixed16 const *pIn,   // Input buffer pointer for holding half fixed point complex values
                             vspa_complex_fixed16 *pOut,        // Output buffer pointer for holding half fixed point complex values
                             vspa_complex_fixed16 const *pBuff, // Base of circular buffer
                             unsigned int const cbuffSize       // Size of circular buffer in half-words
);

// ---------------------------------------------------------------------------
//! @brief           64 pt HFX IFFT using decimation in frequency approach for 16 bit fixed point input data, 16 bit fixed point
//! output data
//!
//! @param[in]       pIn   		Input buffer address
//! @param[out]      pOut  		Output buffer address
//! @param[in]       pBuff 		Base of circular input buffer
//! @param[in]       cbuffSize  Size of circular buffer in half-word units
//! @return          Void.
//! @cycle
//! @stack         	 0
//!
//! This function calculates 64 pt IFFT using decimation in frequency (DIF approach)
//! Equivalent MATLAB command: y = ifft(x)
//!
//! @attention       Inputs must be in linear order
//! @attention       Outputs are in bit-reversed order
//! @attention       The output buffer must be vector-aligned.
// ---------------------------------------------------------------------------
extern void ifftDIF64_hfx_hfx(vspa_complex_fixed16 const *pIn, // Input buffer pointer for holding half fixed point complex values
                              vspa_complex_fixed16 *pOut,      // Output buffer pointer for holding half fixed point complex values
                              vspa_complex_fixed16 const *pBuff, // Base of circular buffer
                              unsigned int const cbuffSize       // Size of circular buffer in half-words
);

// ---------------------------------------------------------------------------
//! @brief           128 pt HFX FFT using decimation in frequency approach for 16 bit fixed point input data, 16 bit fixed point
//! output data
//!
//! @param[in]       pIn   		Input buffer address
//! @param[out]      pOut  		Output buffer address
//! @param[in]       pBuff 		Base of circular input buffer
//! @param[in]       cbuffSize  Size of circular buffer in half-word units
//! @return          Void.
//! @cycle
//! @stack         	 0
//!
//! This function calculates 128 pt FFT using decimation in frequency (DIF approach)
//! Equivalent MATLAB command: y = (1/N)*fft(x)
//!
//! @attention       Inputs must be in linear order
//! @attention       Outputs are in bit-reversed order
//! @attention       The output buffer must be vector-aligned.
// ---------------------------------------------------------------------------
extern void fftDIF128_hfx_hfx(vspa_complex_fixed16 const *pIn, // Input buffer pointer for holding half fixed point complex values
                              vspa_complex_fixed16 *pOut,      // Output buffer pointer for holding half fixed point complex values
                              vspa_complex_fixed16 const *pBuff, // Base of circular buffer
                              unsigned int const cbuffSize       // Size of circular buffer in half-words
);

// ---------------------------------------------------------------------------
//! @brief           128 pt HFX IFFT using decimation in frequency approach for 16 bit fixed point input data, 16 bit fixed point
//! output data
//!
//! @param[in]       pIn   		Input buffer address
//! @param[out]      pOut  		Output buffer address
//! @param[in]       pBuff 		Base of circular input buffer
//! @param[in]       cbuffSize  Size of circular buffer in half-word units
//! @return          Void.
//! @cycle
//! @stack         	 0
//!
//! This function calculates 128 pt IFFT using decimation in frequency (DIF approach)
//! Equivalent MATLAB command: y = ifft(x)
//!
//! @attention       Inputs must be in linear order
//! @attention       Outputs are in bit-reversed order
//! @attention       The output buffer must be vector-aligned.
// ---------------------------------------------------------------------------
extern void ifftDIF128_hfx_hfx(vspa_complex_fixed16 const *pIn, // Input buffer pointer for holding half fixed point complex values
                               vspa_complex_fixed16 *pOut,      // Output buffer pointer for holding half fixed point complex values
                               vspa_complex_fixed16 const *pBuff, // Base of circular buffer
                               unsigned int const cbuffSize       // Size of circular buffer in half-words
);

// ---------------------------------------------------------------------------
//! @brief           256 pt HFX FFT using decimation in frequency approach for 16 bit fixed point input data, 16 bit fixed point
//! output data
//!
//! @param[in]       pIn   		Input buffer address
//! @param[out]      pOut  		Output buffer address
//! @param[in]       pBuff 		Base of circular input buffer
//! @param[in]       cbuffSize  Size of circular buffer in half-word units
//! @return          Void.
//! @cycle
//! @stack         	 0
//!
//! This function calculates 256 pt FFT using decimation in frequency (DIF approach)
//! Equivalent MATLAB command: y = (1/N)*fft(x)
//!
//! @attention       Inputs must be in linear order
//! @attention       Outputs are in bit-reversed order
//! @attention       The output buffer must be vector-aligned.
// ---------------------------------------------------------------------------
extern void fftDIF256_hfx_hfx(vspa_complex_fixed16 const *pIn, // Input buffer pointer for holding half fixed point complex values
                              vspa_complex_fixed16 *pOut,      // Output buffer pointer for holding half fixed point complex values
                              vspa_complex_fixed16 const *pBuff, // Base of circular buffer
                              unsigned int const cbuffSize       // Size of circular buffer in half-words
);

// ---------------------------------------------------------------------------
//! @brief           256 pt HFX IFFT using decimation in frequency approach for 16 bit fixed point input data, 16 bit fixed point
//! output data
//!
//! @param[in]       pIn   		Input buffer address
//! @param[out]      pOut  		Output buffer address
//! @param[in]       pBuff 		Base of circular input buffer
//! @param[in]       cbuffSize  Size of circular buffer in half-word units
//! @return          Void.
//! @cycle
//! @stack         	 0
//!
//! This function calculates 256 pt IFFT using decimation in frequency (DIF approach)
//! Equivalent MATLAB command: y = ifft(x)
//!
//! @attention       Inputs must be in linear order
//! @attention       Outputs are in bit-reversed order
//! @attention       The output buffer must be vector-aligned.
// ---------------------------------------------------------------------------
extern void ifftDIF256_hfx_hfx(vspa_complex_fixed16 const *pIn, // Input buffer pointer for holding half fixed point complex values
                               vspa_complex_fixed16 *pOut,      // Output buffer pointer for holding half fixed point complex values
                               vspa_complex_fixed16 const *pBuff, // Base of circular buffer
                               unsigned int const cbuffSize       // Size of circular buffer in half-words
);

// ---------------------------------------------------------------------------
//! @brief           512 pt HFX FFT using decimation in frequency approach for 16 bit fixed point input data, 16 bit fixed point
//! output data
//!
//! @param[in]       pIn   		Input buffer address
//! @param[out]      pOut  		Output buffer address
//! @param[in]       pBuff 		Base of circular input buffer
//! @param[in]       cbuffSize  Size of circular buffer in half-word units
//! @return          Void.
//! @cycle
//! @stack         	 0
//!
//! This function calculates 512 pt FFT using decimation in frequency (DIF approach)
//! Equivalent MATLAB command: y = (1/N)*fft(x)
//!
//! @attention       Inputs must be in linear order
//! @attention       Outputs are in bit-reversed order
//! @attention       The output buffer must be vector-aligned.
// ---------------------------------------------------------------------------
extern void fftDIF512_hfx_hfx(vspa_complex_fixed16 const *pIn, // Input buffer pointer for holding half fixed point complex values
                              vspa_complex_fixed16 *pOut,      // Output buffer pointer for holding half fixed point complex values
                              vspa_complex_fixed16 const *pBuff, // Base of circular buffer
                              unsigned int const cbuffSize       // Size of circular buffer in half-words
);

// ---------------------------------------------------------------------------
//! @brief           512 pt HFX IFFT using decimation in frequency approach for 16 bit fixed point input data, 16 bit fixed point
//! output data
//!
//! @param[in]       pIn   		Input buffer address
//! @param[out]      pOut  		Output buffer address
//! @param[in]       pBuff 		Base of circular input buffer
//! @param[in]       cbuffSize  Size of circular buffer in half-word units
//! @return          Void.
//! @cycle
//! @stack         	 0
//!
//! This function calculates 512 pt IFFT using decimation in frequency (DIF approach)
//! Equivalent MATLAB command: y = ifft(x)
//!
//! @attention       Inputs must be in linear order
//! @attention       Outputs are in bit-reversed order
//! @attention       The output buffer must be vector-aligned.
// ---------------------------------------------------------------------------
extern void ifftDIF512_hfx_hfx(vspa_complex_fixed16 const *pIn, // Input buffer pointer for holding half fixed point complex values
                               vspa_complex_fixed16 *pOut,      // Output buffer pointer for holding half fixed point complex values
                               vspa_complex_fixed16 const *pBuff, // Base of circular buffer
                               unsigned int const cbuffSize       // Size of circular buffer in half-words
);

// ---------------------------------------------------------------------------
//! @brief           1024 pt HFX FFT using decimation in frequency approach for 16 bit fixed point input data, 16 bit fixed point
//! output data
//!
//! @param[in]       pIn   		Input buffer address
//! @param[out]      pOut  		Output buffer address
//! @param[in]       pBuff 		Base of circular input buffer
//! @param[in]       cbuffSize  Size of circular buffer in half-word units
//! @return          Void.
//! @cycle
//! @stack         	 0
//!
//! This function calculates 1024 pt FFT using decimation in frequency (DIF approach)
//! Equivalent MATLAB command: y = fft(x)
//!
//! @attention       Inputs must be in linear order
//! @attention       Outputs are in bit-reversed order
//! @attention       The output buffer must be vector-aligned.
// ---------------------------------------------------------------------------
extern void fftDIF1024_hfx_hfx(vspa_complex_fixed16 const *pIn, // Input buffer pointer for holding half fixed point complex values
                               vspa_complex_fixed16 *pOut, // Output buffer pointer for holding half floating point complex values
                               vspa_complex_fixed16 const *pBuff, // Base of circular buffer
                               unsigned int const cbuffSize       // Size of circular buffer in half-words
);

// ---------------------------------------------------------------------------
//! @brief           1024 pt HFX IFFT using decimation in frequency approach for 16 bit fixed point input data, 16 bit fixed point
//! output data
//!
//! @param[in]       pIn   		Input buffer address
//! @param[out]      pOut  		Output buffer address
//! @param[in]       pBuff 		Base of circular input buffer
//! @param[in]       cbuffSize  Size of circular buffer in half-word units
//! @return          Void.
//! @cycle
//! @stack         	 0
//!
//! This function calculates 1024 pt IFFT using decimation in frequency (DIF approach)
//! Equivalent MATLAB command: y = N*ifft(x)
//!
//! @attention       Inputs must be in linear order
//! @attention       Outputs are in bit-reversed order
//! @attention       The output buffer must be vector-aligned.
// ---------------------------------------------------------------------------
extern void ifftDIF1024_hfx_hfx(vspa_complex_fixed16 const *pIn, // Input buffer pointer for holding half fixed point complex values
                                vspa_complex_fixed16 *pOut, // Output buffer pointer for holding half floating point complex values
                                vspa_complex_fixed16 const *pBuff, // Base of circular buffer
                                unsigned int const cbuffSize       // Size of circular buffer in half-words
);

// ---------------------------------------------------------------------------
//! @brief           2048 pt HFX FFT using decimation in frequency approach for 16 bit fixed point input data, 16 bit fixed point
//! output data
//!
//! @param[in]       pIn   		Input buffer address
//! @param[out]      pOut  		Output buffer address
//! @param[in]       pBuff 		Base of circular input buffer
//! @param[in]       cbuffSize  Size of circular buffer in half-word units
//! @return          Void.
//! @cycle
//! @stack         	 0
//!
//! This function calculates 2048 pt FFT using decimation in frequency (DIF approach)
//! Equivalent MATLAB command: y = fft(x)
//!
//! @attention       Inputs must be in linear order
//! @attention       Outputs are in bit-reversed order
//! @attention       The output buffer must be vector-aligned.
// ---------------------------------------------------------------------------
extern void fftDIF2048_hfx_hfx(vspa_complex_fixed16 const *pIn, // Input buffer pointer for holding half fixed point complex values
                               vspa_complex_fixed16 *pOut, // Output buffer pointer for holding half floating point complex values
                               vspa_complex_fixed16 const *pBuff, // Base of circular buffer
                               unsigned int const cbuffSize       // Size of circular buffer in half-words
);

// ---------------------------------------------------------------------------
//! @brief           2048 pt HFX IFFT using decimation in frequency approach for 16 bit fixed point input data, 16 bit fixed point
//! output data
//!
//! @param[in]       pIn   		Input buffer address
//! @param[out]      pOut  		Output buffer address
//! @param[in]       pBuff 		Base of circular input buffer
//! @param[in]       cbuffSize  Size of circular buffer in half-word units
//! @return          Void.
//! @cycle
//! @stack         	 0
//!
//! This function calculates 2048 pt IFFT using decimation in frequency (DIF approach)
//! Equivalent MATLAB command: y = N*ifft(x)
//!
//! @attention       Inputs must be in linear order
//! @attention       Outputs are in bit-reversed order
//! @attention       The output buffer must be vector-aligned.
// ---------------------------------------------------------------------------
extern void ifftDIF2048_hfx_hfx(vspa_complex_fixed16 const *pIn, // Input buffer pointer for holding half fixed point complex values
                                vspa_complex_fixed16 *pOut, // Output buffer pointer for holding half floating point complex values
                                vspa_complex_fixed16 const *pBuff, // Base of circular buffer
                                unsigned int const cbuffSize       // Size of circular buffer in half-words
);

// ---------------------------------------------------------------------------
//! @brief           64 pt HFX FFT using decimation in frequency approach for 16 bit fixed point input data, 16 bit floating point
//! output data
//!
//! @param[in]       pIn   		Input buffer address
//! @param[out]      pOut  		Output buffer address
//! @param[in]       pBuff 		Base of circular input buffer
//! @param[in]       cbuffSize  Size of circular buffer in half-word units
//! @return          Void.
//! @cycle
//! @stack         	 0
//!
//! This function calculates 64 pt FFT using decimation in frequency (DIF approach)
//! Equivalent MATLAB command: y = fft(x)
//!
//! @attention       Inputs must be in linear order
//! @attention       Outputs are in bit-reversed order
//! @attention       The output buffer must be vector-aligned.
// ---------------------------------------------------------------------------
extern void fftDIF64_hfx_hfl(vspa_complex_fixed16 const *pIn, // Input buffer pointer for holding half fixed point complex values
                             vspa_complex_float16 *pOut, // Output buffer pointer for holding half floating point complex values
                             vspa_complex_fixed16 const *pBuff, // Base of circular buffer
                             unsigned int const cbuffSize       // Size of circular buffer in half-words
);

// ---------------------------------------------------------------------------
//! @brief           64 pt HFX IFFT using decimation in frequency approach for 16 bit fixed point input data, 16 bit floating point
//! output data
//!
//! @param[in]       pIn   		Input buffer address
//! @param[out]      pOut  		Output buffer address
//! @param[in]       pBuff 		Base of circular input buffer
//! @param[in]       cbuffSize  Size of circular buffer in half-word units
//! @return          Void.
//! @cycle
//! @stack         	 0
//!
//! This function calculates 64 pt IFFT using decimation in frequency (DIF approach)
//! Equivalent MATLAB command: y = N*ifft(x)
//!
//! @attention       Inputs must be in linear order
//! @attention       Outputs are in bit-reversed order
//! @attention       The output buffer must be vector-aligned.
// ---------------------------------------------------------------------------
extern void ifftDIF64_hfx_hfl(vspa_complex_fixed16 const *pIn, // Input buffer pointer for holding half fixed point complex values
                              vspa_complex_float16 *pOut, // Output buffer pointer for holding half floating point complex values
                              vspa_complex_fixed16 const *pBuff, // Base of circular buffer
                              unsigned int const cbuffSize       // Size of circular buffer in half-words
);

// ---------------------------------------------------------------------------
//! @brief           128 pt HFX FFT using decimation in frequency approach for 16 bit fixed point input data, 16 bit floating point
//! output data
//!
//! @param[in]       pIn   		Input buffer address
//! @param[out]      pOut  		Output buffer address
//! @param[in]       pBuff 		Base of circular input buffer
//! @param[in]       cbuffSize  Size of circular buffer in half-word units
//! @return          Void.
//! @cycle
//! @stack         	 0
//!
//! This function calculates 128 pt FFT using decimation in frequency (DIF approach)
//! Equivalent MATLAB command: y = fft(x)
//!
//! @attention       Inputs must be in linear order
//! @attention       Outputs are in bit-reversed order
//! @attention       The output buffer must be vector-aligned.
// ---------------------------------------------------------------------------
extern void fftDIF128_hfx_hfl(vspa_complex_fixed16 const *pIn, // Input buffer pointer for holding half fixed point complex values
                              vspa_complex_float16 *pOut, // Output buffer pointer for holding half floating point complex values
                              vspa_complex_fixed16 const *pBuff, // Base of circular buffer
                              unsigned int const cbuffSize       // Size of circular buffer in half-words
);

// ---------------------------------------------------------------------------
//! @brief           128 pt HFX IFFT using decimation in frequency approach for 16 bit fixed point input data, 16 bit floating point
//! output data
//!
//! @param[in]       pIn   		Input buffer address
//! @param[out]      pOut  		Output buffer address
//! @param[in]       pBuff 		Base of circular input buffer
//! @param[in]       cbuffSize  Size of circular buffer in half-word units
//! @return          Void.
//! @cycle
//! @stack         	 0
//!
//! This function calculates 128 pt IFFT using decimation in frequency (DIF approach)
//! Equivalent MATLAB command: y = N*ifft(x)
//!
//! @attention       Inputs must be in linear order
//! @attention       Outputs are in bit-reversed order
//! @attention       The output buffer must be vector-aligned.
// ---------------------------------------------------------------------------
extern void ifftDIF128_hfx_hfl(vspa_complex_fixed16 const *pIn, // Input buffer pointer for holding half fixed point complex values
                               vspa_complex_float16 *pOut, // Output buffer pointer for holding half floating point complex values
                               vspa_complex_fixed16 const *pBuff, // Base of circular buffer
                               unsigned int const cbuffSize       // Size of circular buffer in half-words
);

// ---------------------------------------------------------------------------
//! @brief           256 pt HFX FFT using decimation in frequency approach for 16 bit fixed point input data, 16 bit floating point
//! output data
//!
//! @param[in]       pIn   		Input buffer address
//! @param[out]      pOut  		Output buffer address
//! @param[in]       pBuff 		Base of circular input buffer
//! @param[in]       cbuffSize  Size of circular buffer in half-word units
//! @return          Void.
//! @cycle
//! @stack         	 0
//!
//! This function calculates 256 pt FFT using decimation in frequency (DIF approach)
//! Equivalent MATLAB command: y = fft(x)
//!
//! @attention       Inputs must be in linear order
//! @attention       Outputs are in bit-reversed order
//! @attention       The output buffer must be vector-aligned.
// ---------------------------------------------------------------------------
extern void fftDIF256_hfx_hfl(vspa_complex_fixed16 const *pIn, // Input buffer pointer for holding half fixed point complex values
                              vspa_complex_float16 *pOut, // Output buffer pointer for holding half floating point complex values
                              vspa_complex_fixed16 const *pBuff, // Base of circular buffer
                              unsigned int const cbuffSize       // Size of circular buffer in half-words
);

// ---------------------------------------------------------------------------
//! @brief           256 pt HFX IFFT using decimation in frequency approach for 16 bit fixed point input data, 16 bit floating point
//! output data
//!
//! @param[in]       pIn   		Input buffer address
//! @param[out]      pOut  		Output buffer address
//! @param[in]       pBuff 		Base of circular input buffer
//! @param[in]       cbuffSize  Size of circular buffer in half-word units
//! @return          Void.
//! @cycle
//! @stack         	 0
//!
//! This function calculates 256 pt IFFT using decimation in frequency (DIF approach)
//! Equivalent MATLAB command: y = N*ifft(x)
//!
//! @attention       Inputs must be in linear order
//! @attention       Outputs are in bit-reversed order
//! @attention       The output buffer must be vector-aligned.
// ---------------------------------------------------------------------------
extern void ifftDIF256_hfx_hfl(vspa_complex_fixed16 const *pIn, // Input buffer pointer for holding half fixed point complex values
                               vspa_complex_float16 *pOut, // Output buffer pointer for holding half floating point complex values
                               vspa_complex_fixed16 const *pBuff, // Base of circular buffer
                               unsigned int const cbuffSize       // Size of circular buffer in half-words
);

// ---------------------------------------------------------------------------
//! @brief           512 pt HFX FFT using decimation in frequency approach for 16 bit fixed point input data, 16 bit floating point
//! output data
//!
//! @param[in]       pIn   		Input buffer address
//! @param[out]      pOut  		Output buffer address
//! @param[in]       pBuff 		Base of circular input buffer
//! @param[in]       cbuffSize  Size of circular buffer in half-word units
//! @return          Void.
//! @cycle
//! @stack         	 0
//!
//! This function calculates 512 pt FFT using decimation in frequency (DIF approach)
//! Equivalent MATLAB command: y = fft(x)
//!
//! @attention       Inputs must be in linear order
//! @attention       Outputs are in bit-reversed order
//! @attention       The output buffer must be vector-aligned.
// ---------------------------------------------------------------------------
extern void fftDIF512_hfx_hfl(vspa_complex_fixed16 const *pIn, // Input buffer pointer for holding half fixed point complex values
                              vspa_complex_float16 *pOut, // Output buffer pointer for holding half floating point complex values
                              vspa_complex_fixed16 const *pBuff, // Base of circular buffer
                              unsigned int const cbuffSize       // Size of circular buffer in half-words
);

// ---------------------------------------------------------------------------
//! @brief           512 pt HFX IFFT using decimation in frequency approach for 16 bit fixed point input data, 16 bit floating point
//! output data
//!
//! @param[in]       pIn   		Input buffer address
//! @param[out]      pOut  		Output buffer address
//! @param[in]       pBuff 		Base of circular input buffer
//! @param[in]       cbuffSize  Size of circular buffer in half-word units
//! @return          Void.
//! @cycle
//! @stack         	 0
//!
//! This function calculates 512 pt IFFT using decimation in frequency (DIF approach)
//! Equivalent MATLAB command: y = N*ifft(x)
//!
//! @attention       Inputs must be in linear order
//! @attention       Outputs are in bit-reversed order
//! @attention       The output buffer must be vector-aligned.
// ---------------------------------------------------------------------------
extern void ifftDIF512_hfx_hfl(vspa_complex_fixed16 const *pIn, // Input buffer pointer for holding half fixed point complex values
                               vspa_complex_float16 *pOut, // Output buffer pointer for holding half floating point complex values
                               vspa_complex_fixed16 const *pBuff, // Base of circular buffer
                               unsigned int const cbuffSize       // Size of circular buffer in half-words
);

// ---------------------------------------------------------------------------
//! @brief           1024 pt HFX FFT using decimation in frequency approach for 16 bit fixed point input data, 16 bit floating point
//! output data
//!
//! @param[in]       pIn   		Input buffer address
//! @param[out]      pOut  		Output buffer address
//! @param[in]       pBuff 		Base of circular input buffer
//! @param[in]       cbuffSize  Size of circular buffer in half-word units
//! @return          Void.
//! @cycle
//! @stack         	 0
//!
//! This function calculates 1024 pt FFT using decimation in frequency (DIF approach)
//! Equivalent MATLAB command: y = fft(x)
//!
//! @attention       Inputs must be in linear order
//! @attention       Outputs are in bit-reversed order
//! @attention       The output buffer must be vector-aligned.
// ---------------------------------------------------------------------------
extern void fftDIF1024_hfx_hfl(vspa_complex_fixed16 const *pIn, // Input buffer pointer for holding half fixed point complex values
                               vspa_complex_float16 *pOut, // Output buffer pointer for holding half floating point complex values
                               vspa_complex_fixed16 const *pBuff, // Base of circular buffer
                               unsigned int const cbuffSize       // Size of circular buffer in half-words
);

// ---------------------------------------------------------------------------
//! @brief           1024 pt HFX IFFT using decimation in frequency approach for 16 bit fixed point input data, 16 bit floating
//! point output data
//!
//! @param[in]       pIn   		Input buffer address
//! @param[out]      pOut  		Output buffer address
//! @param[in]       pBuff 		Base of circular input buffer
//! @param[in]       cbuffSize  Size of circular buffer in half-word units
//! @return          Void.
//! @cycle
//! @stack         	 0
//!
//! This function calculates 1024 pt IFFT using decimation in frequency (DIF approach)
//! Equivalent MATLAB command: y = N*ifft(x)
//!
//! @attention       Inputs must be in linear order
//! @attention       Outputs are in bit-reversed order
//! @attention       The output buffer must be vector-aligned.
// ---------------------------------------------------------------------------
extern void ifftDIF1024_hfx_hfl(vspa_complex_fixed16 const *pIn, // Input buffer pointer for holding half fixed point complex values
                                vspa_complex_float16 *pOut, // Output buffer pointer for holding half floating point complex values
                                vspa_complex_fixed16 const *pBuff, // Base of circular buffer
                                unsigned int const cbuffSize       // Size of circular buffer in half-words
);

// ---------------------------------------------------------------------------
//! @brief           2048 pt HFX FFT using decimation in frequency approach for 16 bit fixed point input data, 16 bit floating point
//! output data
//!
//! @param[in]       pIn   		Input buffer address
//! @param[out]      pOut  		Output buffer address
//! @param[in]       pBuff 		Base of circular input buffer
//! @param[in]       cbuffSize  Size of circular buffer in half-word units
//! @return          Void.
//! @cycle
//! @stack         	 0
//!
//! This function calculates 2048 pt FFT using decimation in frequency (DIF approach)
//! Equivalent MATLAB command: y = fft(x)
//!
//! @attention       Inputs must be in linear order
//! @attention       Outputs are in bit-reversed order
//! @attention       The output buffer must be vector-aligned.
// ---------------------------------------------------------------------------
extern void fftDIF2048_hfx_hfl(vspa_complex_fixed16 const *pIn, // Input buffer pointer for holding half fixed point complex values
                               vspa_complex_float16 *pOut, // Output buffer pointer for holding half floating point complex values
                               vspa_complex_fixed16 const *pBuff, // Base of circular buffer
                               unsigned int const cbuffSize       // Size of circular buffer in half-words
);

// ---------------------------------------------------------------------------
//! @brief           2048 pt HFX IFFT using decimation in frequency approach for 16 bit fixed point input data, 16 bit floating
//! point output data
//!
//! @param[in]       pIn   		Input buffer address
//! @param[out]      pOut  		Output buffer address
//! @param[in]       pBuff 		Base of circular input buffer
//! @param[in]       cbuffSize  Size of circular buffer in half-word units
//! @return          Void.
//! @cycle
//! @stack         	 0
//!
//! This function calculates 2048 pt IFFT using decimation in frequency (DIF approach)
//! Equivalent MATLAB command: y = N*ifft(x)
//!
//! @attention       Inputs must be in linear order
//! @attention       Outputs are in bit-reversed order
//! @attention       The output buffer must be vector-aligned.
// ---------------------------------------------------------------------------
extern void ifftDIF2048_hfx_hfl(vspa_complex_fixed16 const *pIn, // Input buffer pointer for holding half fixed point complex values
                                vspa_complex_float16 *pOut, // Output buffer pointer for holding half floating point complex values
                                vspa_complex_fixed16 const *pBuff, // Base of circular buffer
                                unsigned int const cbuffSize       // Size of circular buffer in half-words
);

//! @} GROUP_FFT

#endif // __DIFFFT_H__
