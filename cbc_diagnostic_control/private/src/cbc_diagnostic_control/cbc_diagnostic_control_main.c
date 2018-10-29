/* Copyright (C) 2018 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @file
 *
 * Retrieves fw, bootloader version and boot timestamps from AIOC
 * 
 */

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>

#include "../../inc/cbc_diagnostic_control/cbc_diagnostic_control_frame_handler.h"
#include "../../inc/cbc_diagnostic_control/cbc_diagnostic_control_options.h"
#include "../../inc/cbc_diagnostic_control/cbc_diagnostic_control_output_flag.h"

int main(int argc, char *argv[])
{
  CbcDiagnosticControlOptions options;

  int fd = 0;
  options.verbose_flag = 0;
  options.output_selection = eIasPrintFlagNone;
  options.boot_timestamps_flag = 0;
  options.log_file_name = "/tmp/IOC_timestamps.txt";

  int32_t result = cbc_diagnostic_parse_option(&options, argc, argv);

  if (result != 0)
  {
    return -2;
  }

  if (options.verbose_flag && options.output_selection == eIasPrintFlagNone
      && options.boot_timestamps_flag == eIasTimestampsNone)
      options.output_selection = eIasPrintFlagAll;

  int const serial_result = cbc_init_devices();

  if (0 != serial_result)
  {
    printf("Unable to configure CBC devices  with error %i\n", serial_result);
    return -1;
  }

  result = cbc_diagnostic_send_request(options.verbose_flag, options.output_selection, options.boot_timestamps_flag);

  if (result == 0)
  {
    result = cbc_diagnostic_receive_answer(options.verbose_flag, options.output_selection, options.boot_timestamps_flag,
                                           options.log_file_name);
  }
  else
  {
    printf("Unable to send request! res %d\n", result);
  }

  cbc_close_devices();
  return result;
}
