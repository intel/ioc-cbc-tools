/* Copyright (C) 2018 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @file
 *
 * Retrieves fw, bootloader version and boot timestamps from AIOC
 * 
 */

#ifndef VEHICLEBUS_CBC_DIAGNOSTIC_OUTPUTFLAG_H
#define VEHICLEBUS_CBC_DIAGNOSTIC_OUTPUTFLAG_H

#ifdef __cplusplus
extern "C" {
#endif

enum IasOutputFlags
{
  eIasPrintFlagNone = 0x00,
  eIasPrintFlagBootloaderVersion = 0x01,
  eIasPrintFlagFirmwareVersion = 0x02,
  eIasPrintFlagMainboardVersion = 0x04,
  eIasPrintFlagAll = 0x1f
};

enum IasTimestampFlags
{
  eIasTimestampsNone = 0x00,
  eIasTimestampsShow = 0x01, eIasTimeStampsLog = 0x02
};

#ifdef __cplusplus
}
#endif

#endif /* VEHICLEBUS_CBC_DIAGNOSTIC_OUTPUTFLAG_H */
