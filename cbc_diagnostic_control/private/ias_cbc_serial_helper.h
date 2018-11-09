/* Copyright (C) 2018 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @file
 *
 * Retrieves fw, bootloader version and boot timestamps from AIOC
 * 
 */

#ifndef VEHICLEBUS_CBCCORE_CBCSERIALHELPER_H
#define VEHICLEBUS_CBCCORE_CBCSERIALHELPER_H

#include "ias_cbc_visibility.h"

/*! \brief enumeration, which holds the available errors */
typedef enum
{
  e_ias_cbc_serial_error_ok                     = 0U,
  e_ias_cbc_serial_error_device_descriptor      = 1U,
  e_ias_cbc_serial_error_baudrate               = 2U,
  e_ias_cbc_serial_error_open_device            = 3U,
  e_ias_cbc_serial_error_device_get_attributes  = 4U,
  e_ias_cbc_serial_error_cf_speed               = 5U,
  e_ias_cbc_serial_error_device_set_attributes  = 6U,
  e_ias_cbc_serial_error_non_blocking           = 7U
}
ias_cbc_serial_error;


/* helper functions to open/close a serial device */

/*
 * Open device with required attributes for IOC communication
 * 
 * @param[out] fd pointer to an integer that will hold the file descriptor after successful opening and initialization of the device
 * @param[in] device_name name of the device to be opened
 * @param[in] baudrate baudrate to be used on the device
 * @param[in] hardware_flow_control use hardware flow control on the device
 */
ias_cbc_serial_error CBC_DSO_PUBLIC cbc_serial_helper_init_serial_device(int * fd,
                                                                         char const * const device_name,
                                                                         unsigned int baudrate,
                                                                         int hardware_flow_control,
                                                                         int open_non_blocking);

#ifdef __cplusplus
}
#endif

#endif // VEHICLEBUS_CBCCORE_CBCSERIALHELPER_H
