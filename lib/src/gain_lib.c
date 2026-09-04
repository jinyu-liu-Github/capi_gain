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

#include "gain_lib.h"
/*=============================================================================
FUNCTION      void example_apply_gain_16

DESCRIPTION   apply gain (Q12) to the input buffer (Q15), shift  and place the result in the
              output buffer (Q15).
OUTPUTS

DEPENDENCIES  None

RETURN VALUE  None

SIDE EFFECTS
===============================================================================*/

void example_apply_gain_16(int16_t *outptr,  /* Pointer to output */
                           int16_t *inptr,   /* Pointer to input */
                           uint16_t gain,    /* Gain in Q12 format */
                           uint32_t samples) /* No of samples to which the gain is to be applied */
{
   int32_t nGainQ15 = ((uint32_t)(gain)) << 3; /* scale the gain up by 3 bit to become Q15 for ease of truncation*/
   while (samples--)
   {
      /* Multiply 32 bit number with 16 bit number*/
      int32_t mul_result = (int32_t)(nGainQ15 * ((int32_t)(*inptr++))); // todo:revert again

      /* Adjust q factor after multiplication */
      mul_result = (mul_result + 0x4000) >> 15;

      /* Saturate to 16 bits*/
      if (mul_result > MAX_16) // 0x7FFF
      {
         *outptr++ = MAX_16;
      }
      else if (mul_result < MIN_16) //-32768
      {
         *outptr++ = MIN_16;
      }
      else
      {
         *outptr++ = (int16_t)mul_result;
      }
   }
}

/*=============================================================================
FUNCTION      void example_apply_gain_32_G1

DESCRIPTION   apply gain (Q12) to the input buffer (Q31), shift  and place the result in the
              output buffer (Q31).
OUTPUTS

DEPENDENCIES  For Gain value greater than 1.

RETURN VALUE  None

SIDE EFFECTS
===============================================================================*/

void example_apply_gain_32_G1(int32_t *outptr,  /* Pointer to output */
                              int32_t *inptr,   /* Pointer to input */
                              uint16_t gain,    /* Gain in Q12 format */
                              uint32_t samples, /* No of samples to which the gain is to be applied */
                              uint16_t qfactor) /* Q factor of data for 24 and 32-bit operation */
{
   int32_t shift_factor = qfactor - 12;                   // shift_factor = 15 for q27 and 19 for q31
   int32_t nGain      = ((uint32_t)gain) << shift_factor; /* scale the gain up by 15 bit to become Q27 for truncation*/
   int32_t round_mask = 0x04000000;                       // Round mask for q27
   if (qfactor == 31)
   {
      round_mask = 0x40000000; // Round mask for q31
   }

   /*--------------------------------------- Non q6 vesion-----------------------------------------------*/
   while (samples--)
   {

      /* Multiply 32 bit number with 32 bit number*/
      int64_t mul_result = (int64_t)(nGain * ((int64_t)(*inptr++)));

      /* round and adjust q factor*/
      mul_result = (mul_result + round_mask) >> qfactor;

      /* Saturate to 32 bits*/
      if (mul_result > MAX_32) // 0x7FFFFFFFL
      {
         *outptr++ = MAX_32;
      }
      else if (mul_result < MIN_32) // 0x80000000L
      {
         *outptr++ = MIN_32;
      }
      else
      {
         *outptr++ = (int32_t)mul_result;
      }
   }
}

/*=============================================================================
FUNCTION      void example_apply_gain_32_L1

DESCRIPTION   apply gain (Q12) to the input buffer (Q31), shift  and place the result in the
              output buffer (Q31).
OUTPUTS

DEPENDENCIES  For Gain value less than 1.

RETURN VALUE  None

SIDE EFFECTS
===============================================================================*/
void example_apply_gain_32_L1(int32_t *outptr,  /* Pointer to output */
                              int32_t *inptr,   /* Pointer to input */
                              uint16_t gain,    /* Gain in Q12 format */
                              uint32_t samples, /* No of samples to which the gain is to be applied */
                              uint16_t qfactor) /* Q factor of data for 24 and 32-bit operation */
{

   int32_t shift_factor = qfactor - 12;         // shift_factor = 15 for q27 and 19 for q31
   int32_t nGain        = gain << shift_factor; /* scale the gain up by 15 bit to become Q27 for truncation*/
   int32_t round_mask   = 0x04000000;           // Round mask for q27
   if (qfactor == 31)
   {
      round_mask = 0x40000000; // Round mask for q31
   }

   while (samples--)
   {

      /* Multiply 32 bit number with 32 bit number*/
      int64_t mul_result = (int64_t)(nGain * ((int64_t)(*inptr++)));

      /* round and adjust q factor*/
      mul_result = (mul_result + round_mask) >> qfactor;

      *outptr++ = (int32_t)mul_result;
   }
}
