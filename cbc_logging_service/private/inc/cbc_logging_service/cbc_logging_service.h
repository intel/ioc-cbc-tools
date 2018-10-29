/* Copyright (C) 2018 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clasue
 *
 * @file
 *
 * This tool receives boot timestamps and dlt logging from AIOC
 *
 */

#ifndef VEHICLEBUS_CBC_DIAGNOSTIC_FRAMEHANDLERFRAMEHANDLER_H
#define VEHICLEBUS_CBC_DIAGNOSTIC_FRAMEHANDLERFRAMEHANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "cbc_logging_service_options.h"

#define DLT_CONVERT_TEXTBUFSIZE 10024

/*! \brief CBC IOC argument types*/
typedef enum
{
  e_ias_cbc_ioc_argument_not_use = 0,
  e_ias_cbc_ioc_argument_type_string,
  e_ias_cbc_ioc_argument_type_bool,
  e_ias_cbc_ioc_argument_type_raw,
  e_ias_cbc_ioc_argument_type_float32_unused, /* Float support has been removed but leaving the value in. */
  e_ias_cbc_ioc_argument_type_int,
  e_ias_cbc_ioc_argument_type_int8,
  e_ias_cbc_ioc_argument_type_int16,
  e_ias_cbc_ioc_argument_type_int32,
  e_ias_cbc_ioc_argument_type_uint8,
  e_ias_cbc_ioc_argument_type_uint16,
  e_ias_cbc_ioc_argument_type_uint32
} ias_cbc_ioc_argument_type;

int cbc_init_device(CbcLoggingServiceControlOptions *);

int run_logging_service(CbcLoggingServiceControlOptions*);

int convert_file(CbcLoggingServiceControlOptions *);

void cbc_close_device();

#ifdef __cplusplus
}
#endif

#endif /* VEHICLEBUS_CBC_DIAGNOSTIC_FRAMEHANDLERFRAMEHANDLER_H */
