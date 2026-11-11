/* =========================================================================
  Copyright (c) 2019-2021, 2023 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Confidential and Proprietary - Qualcomm Technologies, Inc.
 * =========================================================================*/

/**
 * @file capi_gain.c
 * C source file to implement the Gain module.
 *
 * Cleaned-up version of the official capi_gain example for ADSP signal-chain
 * testing:
 *   - All benchmark / performance-measurement code has been removed.
 *   - A sine tone generator has been added: when enabled through the
 *     PARAM_ID_TONE_CONFIG runtime parameter, process() outputs a fixed-point
 *     sine wave (identical tone on every channel) instead of applying gain,
 *     so the module can act as a test signal source inside the chain.
 *
 * Tone generator details:
 *   - Q32 phase accumulator + 256-entry Q15 sine LUT with linear
 *     interpolation (LUT is built once at init).
 *   - Frequency and amplitude are runtime-configurable.
 *     Defaults: disabled, 1000 Hz, amplitude 0.5 (Q15 0x4000).
 *   - Output width follows the operating media format:
 *     16-bit Q15 directly, 32-bit Q27 shifted << 12, 32-bit Q31 shifted << 16.
 */
#include <vector.h>
#include <iostream>
/*------------------------------------------------------------------------
 * Include files
 * -----------------------------------------------------------------------*/
#include "capi_gain.h"
#include "capi_gain_utils.h"
#include <math.h>    /* sinf/floorf: one-time sine LUT init */
#include <string.h>  /* memset */

/*------------------------------------------------------------------------
 * Sine LUT (shared by all instances, built once at init)
 * -----------------------------------------------------------------------*/
#define TONE_LUT_SIZE 256
#define TONE_LUT_SHIFT 8 /* log2(TONE_LUT_SIZE) */
#define TONE_LUT_MASK (TONE_LUT_SIZE - 1)

static int16_t tone_sin_lut[TONE_LUT_SIZE];
static uint32_t tone_sin_lut_ready = 0;

/*------------------------------------------------------------------------
 * Static function declarations
 * -----------------------------------------------------------------------*/
static capi_err_t capi_gain_process(capi_t *_pif,
                                    capi_stream_data_t *input[],
                                    capi_stream_data_t *output[]);

static capi_err_t capi_gain_end(capi_t *_pif);

static capi_err_t capi_gain_set_param(capi_t *                _pif,
                                      uint32_t                param_id,
                                      const capi_port_info_t *port_info_ptr,
                                      capi_buf_t *            params_ptr);

static capi_err_t capi_gain_get_param(capi_t *                _pif,
                                      uint32_t                param_id,
                                      const capi_port_info_t *port_info_ptr,
                                      capi_buf_t *            params_ptr);

static capi_err_t capi_gain_set_properties(capi_t *_pif,
                                           capi_proplist_t *props_ptr);

static capi_err_t capi_gain_get_properties(capi_t *_pif,
                                           capi_proplist_t *props_ptr);

/* Function table for gain module */
static capi_vtbl_t vtbl = {
   capi_gain_process,
   capi_gain_end,
   capi_gain_set_param,
   capi_gain_get_param,
   capi_gain_set_properties,
   capi_gain_get_properties
};

/* -------------------------------------------------------------------------
 * Tone generator helpers
 * -------------------------------------------------------------------------*/

/* Build the shared Q15 sine LUT. Called once from capi_gain_init. */
static void capi_gain_tone_lut_init(void)
{
   if (0 != tone_sin_lut_ready)
   {
      return;
   }

   for (uint32_t i = 0; i < TONE_LUT_SIZE; i++)
   {
      /* One full sine period mapped over the LUT. */
      float phase = ((float)i * 6.28318530718f) / (float)TONE_LUT_SIZE;
      float value = sinf(phase);

      /* Round to nearest Q15 value. */
      tone_sin_lut[i] = (int16_t)floorf((value * 32767.0f) + 0.5f);
   }

   tone_sin_lut_ready = 1;
}

/* Q32 phase increment: (freq / sample_rate) * 2^32 in 64-bit math. */
static uint32_t capi_gain_tone_phase_inc(uint32_t freq_hz, uint32_t sample_rate)
{
   return (uint32_t)(((uint64_t)freq_hz << 32) / sample_rate);
}

/* (Re)compute the phase increment when tone settings or sample rate change. */
static void capi_gain_tone_update_phase_inc(capi_gain_t *me_ptr)
{
   capi_gain_tone_state_t *tone = &me_ptr->tone_state;
   uint32_t sample_rate = me_ptr->operating_media_fmt.format.sampling_rate;

   if (0 == tone->enable)
   {
      return;
   }

   if ((sample_rate == tone->sample_rate) &&
       (tone->freq_hz == tone->cached_freq_hz))
   {
      return;
   }

   /* Keep the tone strictly below Nyquist. */
   uint32_t freq = tone->freq_hz;
   if ((0 == freq) || (freq >= (sample_rate >> 1)))
   {
      freq = (sample_rate >> 1) - 1;
      AR_MSG(DBG_HIGH_PRIO,
             "CAPI GAIN: Tone freq %lu Hz out of range for %lu Hz sample rate, clamping to %lu Hz",
             tone->freq_hz, sample_rate, freq);
   }

   tone->sample_rate    = sample_rate;
   tone->cached_freq_hz = freq;
   tone->phase_inc      = capi_gain_tone_phase_inc(freq, sample_rate);
}

/* Generate the next tone sample (Q15) and advance the phase accumulator. */
static int16_t capi_gain_tone_next_sample(capi_gain_tone_state_t *tone)
{
   /* Top 8 bits of the Q32 phase select the LUT entry, next 8 bits the
    * interpolation fraction. */
   uint32_t idx  = tone->phase_acc >> (32 - TONE_LUT_SHIFT);
   uint32_t frac = (tone->phase_acc >> (32 - TONE_LUT_SHIFT - 8)) & 0xFF;

   int32_t base = tone_sin_lut[idx];
   int32_t next = tone_sin_lut[(idx + 1) & TONE_LUT_MASK];

   /* Linear interpolation between LUT entries, both in Q15. */
   int32_t interp = base + (((next - base) * (int32_t)frac) >> 8);

   /* Apply Q15 amplitude and advance the phase. */
   int16_t sample = (int16_t)((interp * (int32_t)tone->amplitude_q15) >> 15);

   tone->phase_acc += tone->phase_inc;

   return sample;
}

/* Raise a process-state event based on gain config and tone mode.
 * In tone mode the module must always be processed, regardless of gain. */
static void capi_gain_update_process_event(capi_gain_t *me_ptr)
{
   uint32_t enable = TRUE;

   if (0 == me_ptr->tone_state.enable)
   {
      enable =
         ((Q13_UNITY_GAIN != (me_ptr->gain_config.gain_q12 << 1)) &&
          (0 != me_ptr->gain_config.enable));
   }

   (void)capi_gain_raise_process_event(me_ptr, enable);
}

/* -------------------------------------------------------------------------
 * Function name: capi_gain_get_static_properties
 * -------------------------------------------------------------------------*/
capi_err_t capi_gain_get_static_properties(capi_proplist_t *init_set_properties,
                                           capi_proplist_t *static_properties)
{
   capi_err_t capi_result = CAPI_EOK;

   if (NULL != static_properties)
   {
      capi_result =
         capi_gain_process_get_properties((capi_gain_t *)NULL,
                                          static_properties);

      if (CAPI_FAILED(capi_result))
      {
         AR_MSG(DBG_ERROR_PRIO,
                "CAPI GAIN: get static properties failed!");
         return capi_result;
      }
      else
      {
         AR_MSG(DBG_HIGH_PRIO,
                "CAPI GAIN: get static properties successful");
      }
   }
   else
   {
      AR_MSG(DBG_ERROR_PRIO,
             "CAPI GAIN: Get static properties received NULL bad pointer");
   }

   return capi_result;
}

/*------------------------------------------------------------------------
 * Function name: capi_gain_init
 * -----------------------------------------------------------------------*/
capi_err_t capi_gain_init(capi_t *_pif,
                          capi_proplist_t *init_set_properties)
{
   capi_err_t capi_result = CAPI_EOK;

   if (NULL == _pif || NULL == init_set_properties)
   {
      AR_MSG(DBG_ERROR_PRIO,
             "CAPI GAIN: Init received NULL pointers, failing");
      return CAPI_EBADPARAM;
   }

   capi_gain_t *me_ptr = (capi_gain_t *)_pif;

   memset(me_ptr, 0, sizeof(capi_gain_t));
   me_ptr->vtbl.vtbl_ptr = &vtbl;

   capi_gain_module_init_media_fmt_v2(&me_ptr->operating_media_fmt);

   /* Default gain: unity gain (Q13 0x2000). */
   uint16_t apply_gain          = 0x2000;
   me_ptr->gain_config.gain_q12 = apply_gain >> 1;
   me_ptr->gain_config.gain_q13 = apply_gain;

   /* Default tone generator: disabled, 1 kHz, amplitude 0.5 (Q15). */
   me_ptr->tone_state.enable        = 0;
   me_ptr->tone_state.freq_hz       = TONE_DEFAULT_FREQ_HZ;
   me_ptr->tone_state.amplitude_q15 = TONE_DEFAULT_AMPLITUDE_Q15;

   /* Build the shared sine LUT once (first instance to init). */
   capi_gain_tone_lut_init();

   AR_MSG(DBG_HIGH_PRIO,
          "CAPI GAIN: Init - gain_q12=0x%04X, gain_q13=0x%04X, tone_enable=%lu, tone_freq=%lu Hz, tone_amp_q15=0x%04X",
          me_ptr->gain_config.gain_q12,
          me_ptr->gain_config.gain_q13,
          (uint32_t)me_ptr->tone_state.enable,
          (uint32_t)me_ptr->tone_state.freq_hz,
          (uint32_t)me_ptr->tone_state.amplitude_q15);

   capi_result = capi_gain_process_set_properties(me_ptr,
                                                  init_set_properties);

   if (CAPI_FAILED(capi_result))
   {
      AR_MSG(DBG_ERROR_PRIO,
             "CAPI GAIN: Initialization Set Property Failed");
      return capi_result;
   }

   AR_MSG(DBG_HIGH_PRIO,
          "CAPI GAIN: Initialization completed !!");

   return capi_result;
}

/* -------------------------------------------------------------------------
 * Function name: capi_gain_process
 * -------------------------------------------------------------------------*/
static capi_err_t capi_gain_process(capi_t *_pif,
                                    capi_stream_data_t *input[],
                                    capi_stream_data_t *output[])
{
   capi_err_t capi_result = CAPI_EOK;

   if ((NULL == _pif) ||
       (NULL == output) ||
       (NULL == output[0]) ||
       (NULL == output[0]->buf_ptr))
   {
      AR_MSG(DBG_ERROR_PRIO,
             "CAPI GAIN: Process: invalid output buffer");
      return CAPI_EFAILED;
   }

   capi_gain_t *me_ptr = (capi_gain_t *)_pif;

   uint32_t tone_enabled = me_ptr->tone_state.enable;

   /* The input is only required in normal gain mode. */
   if ((0 == tone_enabled) &&
       ((NULL == input) ||
        (NULL == input[0]) ||
        (NULL == input[0]->buf_ptr)))
   {
      AR_MSG(DBG_ERROR_PRIO,
             "CAPI GAIN: Process: invalid input buffer");
      return CAPI_EFAILED;
   }

   int32_t byte_sample_convert =
      (BIT_WIDTH_16 == me_ptr->operating_media_fmt.format.bits_per_sample)
         ? 1
         : 2;

   uint32_t samples_to_process = 0;

   if (0 != tone_enabled)
   {
      /* Tone mode ignores the input; the output length drives the frame. */
      samples_to_process =
         output[0]->buf_ptr[0].max_data_len >> byte_sample_convert;

      capi_gain_tone_update_phase_inc(me_ptr);
   }
   else
   {
      int32_t inp_samples =
         input[0]->buf_ptr[0].actual_data_len >> byte_sample_convert;

      int32_t out_samples =
         output[0]->buf_ptr[0].max_data_len >> byte_sample_convert;

      samples_to_process =
         (inp_samples < out_samples) ? inp_samples : out_samples;
   }

   for (uint32_t ch = 0;
        ch < me_ptr->operating_media_fmt.format.num_channels;
        ch++)
   {
      if (NULL == output[0]->buf_ptr[ch].data_ptr)
      {
         AR_MSG(DBG_ERROR_PRIO,
                "CAPI GAIN: Process: NULL output data_ptr at ch=%lu",
                (uint32_t)ch);
         return CAPI_EFAILED;
      }

      if (0 != tone_enabled)
      {
         /* Generate the sine tone (identical tone on every channel). */
         if (BIT_WIDTH_16 ==
             me_ptr->operating_media_fmt.format.bits_per_sample)
         {
            int16_t *out_ptr = (int16_t *)output[0]->buf_ptr[ch].data_ptr;

            for (uint32_t s = 0; s < samples_to_process; s++)
            {
               out_ptr[s] = capi_gain_tone_next_sample(&me_ptr->tone_state);
            }

            output[0]->buf_ptr[ch].actual_data_len =
               samples_to_process << 1;
         }
         else
         {
            int32_t *out_ptr = (int32_t *)output[0]->buf_ptr[ch].data_ptr;

            /* Q15 -> Q27 (<<12) or Q15 -> Q31 (<<16). */
            uint32_t shift =
               (31 == me_ptr->operating_media_fmt.format.q_factor)
                  ? 16
                  : 12;

            for (uint32_t s = 0; s < samples_to_process; s++)
            {
               out_ptr[s] =
                  ((int32_t)capi_gain_tone_next_sample(&me_ptr->tone_state))
                  << shift;
            }

            output[0]->buf_ptr[ch].actual_data_len =
               samples_to_process << 2;
         }
      }
      else
      {
         /* Normal gain processing. */
         if (NULL == input[0]->buf_ptr[ch].data_ptr)
         {
            AR_MSG(DBG_ERROR_PRIO,
                   "CAPI GAIN: Process: NULL input data_ptr at ch=%lu",
                   (uint32_t)ch);
            return CAPI_EFAILED;
         }

         if (BIT_WIDTH_16 ==
             me_ptr->operating_media_fmt.format.bits_per_sample)
         {
            example_apply_gain_16((int16_t *)output[0]->buf_ptr[ch].data_ptr,
                                  (int16_t *)input[0]->buf_ptr[ch].data_ptr,
                                  me_ptr->gain_config.gain_q12,
                                  samples_to_process);

            output[0]->buf_ptr[ch].actual_data_len =
               samples_to_process << 1;

            input[0]->buf_ptr[ch].actual_data_len =
               samples_to_process << 1;
         }
         else
         {
            if ((uint16_t)(me_ptr->gain_config.gain_q12 >> 12) > 0)
            {
               example_apply_gain_32_G1((int32_t *)output[0]->buf_ptr[ch].data_ptr,
                                        (int32_t *)input[0]->buf_ptr[ch].data_ptr,
                                        me_ptr->gain_config.gain_q12,
                                        samples_to_process,
                                        me_ptr->operating_media_fmt.format.q_factor);
            }
            else
            {
               example_apply_gain_32_L1((int32_t *)output[0]->buf_ptr[ch].data_ptr,
                                        (int32_t *)input[0]->buf_ptr[ch].data_ptr,
                                        me_ptr->gain_config.gain_q12,
                                        samples_to_process,
                                        me_ptr->operating_media_fmt.format.q_factor);
            }

            output[0]->buf_ptr[ch].actual_data_len =
               samples_to_process << 2;

            input[0]->buf_ptr[ch].actual_data_len =
               samples_to_process << 2;
         }
      }
   }

   if (samples_to_process)
   {
      if ((NULL != input) && (NULL != input[0]))
      {
         output[0]->flags = input[0]->flags;

         if (input[0]->flags.is_timestamp_valid)
         {
            output[0]->timestamp =
               input[0]->timestamp - me_ptr->events_info.delay_in_us;
         }
      }
   }

   return capi_result;
}

/*------------------------------------------------------------------------
 * Function name: capi_gain_end
 * -----------------------------------------------------------------------*/
static capi_err_t capi_gain_end(capi_t *_pif)
{
   capi_err_t capi_result = CAPI_EOK;

   if (NULL == _pif)
   {
      CAPI_SET_ERROR(capi_result, CAPI_EBADPARAM);
      return capi_result;
   }

   capi_gain_t *me_ptr = (capi_gain_t *)_pif;

   AR_MSG(DBG_HIGH_PRIO,
          "CAPI GAIN: End - gain_q12=0x%04X, gain_q13=0x%04X, enable=%lu, tone_enable=%lu",
          me_ptr->gain_config.gain_q12,
          me_ptr->gain_config.gain_q13,
          (uint32_t)me_ptr->gain_config.enable,
          (uint32_t)me_ptr->tone_state.enable);

   me_ptr->vtbl.vtbl_ptr = NULL;

   AR_MSG(DBG_HIGH_PRIO,
          "CAPI GAIN: End done");

   return capi_result;
}

/* -------------------------------------------------------------------------
 * Function name: capi_gain_set_param
 * -------------------------------------------------------------------------*/
static capi_err_t capi_gain_set_param(capi_t *                _pif,
                                      uint32_t                param_id,
                                      const capi_port_info_t *port_info_ptr,
                                      capi_buf_t *            params_ptr)
{
   capi_err_t capi_result = CAPI_EOK;

   if (NULL == _pif || NULL == params_ptr)
   {
      AR_MSG(DBG_ERROR_PRIO,
             "CAPI GAIN: Set param - NULL pointer");
      return CAPI_EBADPARAM;
   }

   capi_gain_t *me_ptr = (capi_gain_t *)(_pif);

   AR_MSG(DBG_HIGH_PRIO,
          "CAPI GAIN: Set param request - param_id=0x%08X, data_len=%lu",
          param_id,
          params_ptr->actual_data_len);

   switch (param_id)
   {
      case PARAM_ID_MODULE_ENABLE:
      {
         if (params_ptr->actual_data_len >=
             sizeof(param_id_module_enable_t))
         {
            param_id_module_enable_t *gain_module_enable_ptr =
               (param_id_module_enable_t *)(params_ptr->data_ptr);

            AR_MSG(DBG_HIGH_PRIO,
                   "CAPI GAIN: Set ENABLE - old=%lu, new=%lu",
                   (uint32_t)me_ptr->gain_config.enable,
                   (uint32_t)gain_module_enable_ptr->enable);

            me_ptr->gain_config.enable =
               gain_module_enable_ptr->enable;

            capi_gain_update_process_event(me_ptr);
         }
         else
         {
            AR_MSG(DBG_ERROR_PRIO,
                   "CAPI GAIN: Set ENABLE - Bad param size %lu, expected %lu",
                   params_ptr->actual_data_len,
                   (uint32_t)sizeof(param_id_module_enable_t));

            capi_result |= CAPI_ENEEDMORE;
         }
         break;
      }

      case PARAM_ID_GAIN_MODULE_GAIN:
      {
         if (params_ptr->actual_data_len >=
             sizeof(param_id_module_gain_cfg_t))
         {
            param_id_module_gain_cfg_t *gain_cfg_ptr =
               (param_id_module_gain_cfg_t *)(params_ptr->data_ptr);

            AR_MSG(DBG_HIGH_PRIO,
                   "CAPI GAIN: Set GAIN - old_q13=0x%04X old_q12=0x%04X, new_q13=0x%04X new_q12=0x%04X",
                   me_ptr->gain_config.gain_q13,
                   me_ptr->gain_config.gain_q12,
                   gain_cfg_ptr->gain,
                   gain_cfg_ptr->gain >> 1);

            me_ptr->gain_config.gain_q13 = gain_cfg_ptr->gain;
            me_ptr->gain_config.gain_q12 = gain_cfg_ptr->gain >> 1;

            capi_gain_update_process_event(me_ptr);
         }
         else
         {
            AR_MSG(DBG_ERROR_PRIO,
                   "CAPI GAIN: Set GAIN - Bad param size %lu, expected %lu",
                   params_ptr->actual_data_len,
                   (uint32_t)sizeof(param_id_module_gain_cfg_t));

            capi_result |= CAPI_ENEEDMORE;
         }
         break;
      }

      case PARAM_ID_TONE_CONFIG:
      {
         if (params_ptr->actual_data_len >= sizeof(param_id_tone_config_t))
         {
            param_id_tone_config_t *tone_cfg_ptr =
               (param_id_tone_config_t *)(params_ptr->data_ptr);

            if ((tone_cfg_ptr->amplitude_q15 > TONE_MAX_AMPLITUDE_Q15) ||
                (tone_cfg_ptr->freq_hz > TONE_MAX_FREQ_HZ))
            {
               AR_MSG(DBG_ERROR_PRIO,
                      "CAPI GAIN: Set TONE - bad amplitude 0x%04X or frequency %lu Hz",
                      (uint32_t)tone_cfg_ptr->amplitude_q15,
                      (uint32_t)tone_cfg_ptr->freq_hz);
               CAPI_SET_ERROR(capi_result, CAPI_EBADPARAM);
               break;
            }

            AR_MSG(DBG_HIGH_PRIO,
                   "CAPI GAIN: Set TONE - enable=%lu, freq=%lu Hz, amp_q15=0x%04X",
                   (uint32_t)tone_cfg_ptr->enable,
                   (uint32_t)tone_cfg_ptr->freq_hz,
                   (uint32_t)tone_cfg_ptr->amplitude_q15);

            /* Restart the tone from phase 0 on a rising enable edge so
             * every test run starts from a known sample. */
            if ((0 == me_ptr->tone_state.enable) &&
                (0 != tone_cfg_ptr->enable))
            {
               me_ptr->tone_state.phase_acc = 0;
            }

            me_ptr->tone_state.enable        = tone_cfg_ptr->enable;
            me_ptr->tone_state.freq_hz       = tone_cfg_ptr->freq_hz;
            me_ptr->tone_state.amplitude_q15 = tone_cfg_ptr->amplitude_q15;

            capi_gain_update_process_event(me_ptr);
         }
         else
         {
            AR_MSG(DBG_ERROR_PRIO,
                   "CAPI GAIN: Set TONE - Bad param size %lu, expected %lu",
                   params_ptr->actual_data_len,
                   (uint32_t)sizeof(param_id_tone_config_t));

            capi_result |= CAPI_ENEEDMORE;
         }
         break;
      }

      default:
      {
         AR_MSG(DBG_ERROR_PRIO,
                "CAPI GAIN: Set param - unsupported param_id=0x%08X",
                param_id);

         CAPI_SET_ERROR(capi_result, CAPI_EUNSUPPORTED);
         break;
      }
   }

   return capi_result;
}

/* -------------------------------------------------------------------------
 * Function name: capi_gain_get_param
 * -------------------------------------------------------------------------*/
static capi_err_t capi_gain_get_param(capi_t *                _pif,
                                      uint32_t                param_id,
                                      const capi_port_info_t *port_info_ptr,
                                      capi_buf_t *            params_ptr)
{
   capi_err_t capi_result = CAPI_EOK;

   if (NULL == _pif || NULL == params_ptr)
   {
      AR_MSG(DBG_ERROR_PRIO,
             "CAPI GAIN: Get param - NULL pointer, param_id=0x%08X",
             param_id);
      return CAPI_EBADPARAM;
   }

   capi_gain_t *me_ptr = (capi_gain_t *)_pif;

   switch (param_id)
   {
      case PARAM_ID_MODULE_ENABLE:
      {
         if (params_ptr->max_data_len >=
             sizeof(param_id_module_enable_t))
         {
            param_id_module_enable_t *enable_ptr =
               (param_id_module_enable_t *)(params_ptr->data_ptr);

            enable_ptr->enable = me_ptr->gain_config.enable;

            params_ptr->actual_data_len =
               (uint32_t)sizeof(param_id_module_enable_t);

            AR_MSG(DBG_HIGH_PRIO,
                   "CAPI GAIN: Get ENABLE - enable=%lu",
                   (uint32_t)enable_ptr->enable);
         }
         else
         {
            AR_MSG(DBG_ERROR_PRIO,
                   "CAPI GAIN: Get ENABLE - Bad payload size %lu, expected %lu",
                   params_ptr->max_data_len,
                   (uint32_t)sizeof(param_id_module_enable_t));

            capi_result = CAPI_ENEEDMORE;
         }
         break;
      }

      case PARAM_ID_GAIN_MODULE_GAIN:
      {
         if (params_ptr->max_data_len >=
             sizeof(param_id_module_gain_cfg_t))
         {
            param_id_module_gain_cfg_t *gain_cfg_ptr =
               (param_id_module_gain_cfg_t *)(params_ptr->data_ptr);

            gain_cfg_ptr->gain     = me_ptr->gain_config.gain_q13;
            gain_cfg_ptr->reserved = 0;

            params_ptr->actual_data_len =
               (uint32_t)sizeof(param_id_module_gain_cfg_t);

            AR_MSG(DBG_HIGH_PRIO,
                   "CAPI GAIN: Get GAIN - gain_q13=0x%04X",
                   gain_cfg_ptr->gain);
         }
         else
         {
            AR_MSG(DBG_ERROR_PRIO,
                   "CAPI GAIN: Get GAIN - Bad param size %lu, expected %lu",
                   params_ptr->max_data_len,
                   (uint32_t)sizeof(param_id_module_gain_cfg_t));

            capi_result = CAPI_ENEEDMORE;
         }
         break;
      }

      case PARAM_ID_TONE_CONFIG:
      {
         if (params_ptr->max_data_len >= sizeof(param_id_tone_config_t))
         {
            param_id_tone_config_t *tone_cfg_ptr =
               (param_id_tone_config_t *)(params_ptr->data_ptr);

            tone_cfg_ptr->enable        = me_ptr->tone_state.enable;
            tone_cfg_ptr->freq_hz       = me_ptr->tone_state.freq_hz;
            tone_cfg_ptr->amplitude_q15 = me_ptr->tone_state.amplitude_q15;
            tone_cfg_ptr->reserved      = 0;

            params_ptr->actual_data_len =
               (uint32_t)sizeof(param_id_tone_config_t);

            AR_MSG(DBG_HIGH_PRIO,
                   "CAPI GAIN: Get TONE - enable=%lu, freq=%lu Hz, amp_q15=0x%04X",
                   (uint32_t)tone_cfg_ptr->enable,
                   (uint32_t)tone_cfg_ptr->freq_hz,
                   (uint32_t)tone_cfg_ptr->amplitude_q15);
         }
         else
         {
            AR_MSG(DBG_ERROR_PRIO,
                   "CAPI GAIN: Get TONE - Bad payload size %lu, expected %lu",
                   params_ptr->max_data_len,
                   (uint32_t)sizeof(param_id_tone_config_t));

            capi_result = CAPI_ENEEDMORE;
         }
         break;
      }

      default:
      {
         AR_MSG(DBG_ERROR_PRIO,
                "CAPI GAIN: Get param - unsupported param_id=0x%08X",
                param_id);

         capi_result |= CAPI_EUNSUPPORTED;
         break;
      }
   }

   return capi_result;
}

/* -------------------------------------------------------------------------
 * Function name: capi_gain_set_properties
 * -------------------------------------------------------------------------*/
static capi_err_t capi_gain_set_properties(capi_t *_pif,
                                           capi_proplist_t *props_ptr)
{
   capi_err_t capi_result = CAPI_EOK;

   if (NULL == _pif)
   {
      AR_MSG(DBG_ERROR_PRIO,
             "CAPI GAIN: Set properties - NULL module pointer");
      return CAPI_EBADPARAM;
   }

   capi_gain_t *me_ptr = (capi_gain_t *)_pif;

   capi_result = capi_gain_process_set_properties(me_ptr, props_ptr);

   return capi_result;
}

/* -------------------------------------------------------------------------
 * Function name: capi_gain_get_properties
 * -------------------------------------------------------------------------*/
static capi_err_t capi_gain_get_properties(capi_t *_pif,
                                           capi_proplist_t *props_ptr)
{
   if (NULL == _pif)
   {
      AR_MSG(DBG_ERROR_PRIO,
             "CAPI GAIN: Get properties - NULL module pointer");
      return CAPI_EBADPARAM;
   }

   capi_gain_t *me_ptr = (capi_gain_t *)_pif;

   return capi_gain_process_get_properties(me_ptr, props_ptr);
}
