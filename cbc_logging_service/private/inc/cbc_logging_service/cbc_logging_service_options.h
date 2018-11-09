/* Copyright (C) 2018 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clasue
 *
 * @file
 *
 * This tool receives boot timestamps and dlt logging from AIOC
 *
 */

#ifndef VEHICLEBUS_CBC_DIAGNOSTIC_OPTIONS_H
#define VEHICLEBUS_CBC_DIAGNOSTIC_OPTIONS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

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

/* handling of command-line options for cbc_diagnostic_control */
typedef struct CbcLoggingServiceControlOptions
{
    uint8_t verbose_flag;
    uint8_t output_selection;
    uint8_t boot_timestamps_flag;
    uint8_t dlt_log_level;
    uint8_t dlt_prints;
    uint8_t convert;
    char* btstamps_file;
    char* dlt_file;
    char* txt_file;
} CbcLoggingServiceControlOptions;

void cbc_logging_print_help();


/**
 * @return 0 on success
 * @return -1 on failure
 * @return 1 if help was requested
 */
int32_t cbc_logging_parse_option(CbcLoggingServiceControlOptions * options, int argc, char *argv[]);


#ifdef __cplusplus
}
#endif

#endif /* VEHICLEBUS_CBC_DIAGNOSTIC_OPTIONS_H */
