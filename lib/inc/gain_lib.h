/**
@file gain_module_lib.h
   Header file to implement the Gain module library*/

/* =========================================================================
  Copyright (c) 2019-2021, 2023 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Confidential and Proprietary - Qualcomm Technologies, Inc.
  ========================================================================== */

/* =========================================================================
 * Edit History:
 * when         who         what, where, why
 * ----------   -------     -------------------------------------

   ========================================================================= */

#ifndef _GAIN_MODULE_LIB_H
#define _GAIN_MODULE_LIB_H

/*------------------------------------------------------------------------
 * Include files
 * -----------------------------------------------------------------------*/
#include "ar_defs.h"
#include "posal.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/*------------------------------------------------------------------------
 * Macros
 * -----------------------------------------------------------------------*/
/** Maximum value of a signed 32-bit integer.
 */
static const int32_t MAX_32 = 0x7FFFFFFFL;

/** Minimum value of a signed 32-bit integer.
 */
static const int32_t MIN_32 = 0x80000000L;

/** Maximum value of a signed 16-bit integer.
 */
static const int16_t MAX_16 = 0x7FFF;

/** Minimum value of a signed 16-bit integer.
 */
static const int16_t MIN_16 = -32768; // 0x8000; // this removes truncation warning

/*------------------------------------------------------------------------
 * Function declarations
 * -----------------------------------------------------------------------*/

void example_apply_gain_16(int16_t *outptr,   /* Pointer to output */
                           int16_t *inptr,    /* Pointer to input */
                           uint16_t gain,     /* Gain in Q12 format */
                           uint32_t samples); /* No of samples to which the gain is to be applied */

void example_apply_gain_32_G1(int32_t *outptr,   /* Pointer to output */
                              int32_t *inptr,    /* Pointer to input */
                              uint16_t gain,     /* Gain in Q12 format */
                              uint32_t samples,  /* No of samples to which the gain is to be applied */
                              uint16_t qfactor); /* Qfactor of data to identify 24 vs 32 bit sample data */

void example_apply_gain_32_L1(int32_t *outptr,   /* Pointer to output */
                              int32_t *inptr,    /* Pointer to input */
                              uint16_t gain,     /* Gain in Q12 format */
                              uint32_t samples,  /* No of samples to which the gain is to be applied */
                              uint16_t qfactor); /* Qfactor of data to identify 24 vs 32 bit sample data */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // _GAIN_MODULE_LIB_H
