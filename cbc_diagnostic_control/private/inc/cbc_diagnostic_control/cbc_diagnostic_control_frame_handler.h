/* Copyright (C) 2018 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @file
 *
 * Retrieves fw, bootloader version and boot timestamps from AIOC
 * 
 */

#ifndef VEHICLEBUS_CBC_DIAGNOSTIC_FRAMEHANDLERFRAMEHANDLER_H
#define VEHICLEBUS_CBC_DIAGNOSTIC_FRAMEHANDLERFRAMEHANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

int cbc_init_devices();

int cbc_diagnostic_send_request(uint8_t verbose, uint8_t output_selection, uint8_t boot_timestamps_flag);

int cbc_diagnostic_receive_answer(uint8_t verbose, uint8_t output_flags, uint8_t boot_timestamps_flag,
                                  const char* log_file);

void cbc_close_devices();

#ifdef __cplusplus
}
#endif

#endif /* VEHICLEBUS_CBC_DIAGNOSTIC_FRAMEHANDLERFRAMEHANDLER_H */
