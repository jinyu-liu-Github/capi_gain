#ifndef CAPI_GAIN_H
#define CAPI_GAIN_H

/* ======================================================================== */
/**
@file capi_module.h
Header file to implement the Gain block*/

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

/*------------------------------------------------------------------------
 * Include files
 * -----------------------------------------------------------------------*/
#include "capi.h"
#include "ar_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------------
 * Capi entry point functions
 * -----------------------------------------------------------------------*/
/**
 * Get static properties of Gain module such as
 * memory, stack requirements etc.
 */
capi_err_t capi_gain_get_static_properties(capi_proplist_t *init_set_properties, capi_proplist_t *static_properties);

/**
 * Instantiates(and allocates) the module memory.
 */
capi_err_t capi_gain_init(capi_t *_pif, capi_proplist_t *init_set_properties);

#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif // CAPI_GAIN_H
