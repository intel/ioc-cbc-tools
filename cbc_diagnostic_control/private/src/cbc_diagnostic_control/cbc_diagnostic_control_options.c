/*
 @COPYRIGHT_TAG@
 */
/**
 * @file
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../../inc/cbc_diagnostic_control/cbc_diagnostic_control_options.h"
#include "../../inc/cbc_diagnostic_control/cbc_diagnostic_control_output_flag.h"
#include "../../inc/cbc_diagnostic_control/generated/version.h"

/* Original output:
 Usage:
 ./vehicle_bus/cbc_diagnostic/cbc_diagnostic [OPTION...] serialDeviceName
 Intel reference design version check utility (raw serial access)

 Help Options:
 -h, --help                    Show help

 Application Options:
 -b, --bootloaderVersion       Print the bootloader version.
 -f, --firmwareVersion         Print the firmware version.
 -m, --mainboardVersion        Print the mainboard version.
 -u, --hardwareFlowControl     Use hardware flow control on the serial device.
 *
 */

struct cbc_diagnostic_control_options
{
  char const * const short_opt;
  char const * const long_opt;
  char const * const help_text;
};

static struct cbc_diagnostic_control_options cbc_diagnostic_control_options[] =
{
  { "-h", "--help", "Show help" },
  { "-v", "--verbose", "Be verbose" }, /* optional parameter? */
  { "-b", "--bootloaderVersion", "Print version of the bootloader" },
  { "-f", "--firmwareVersion", "Print version of the firmware" },
  { "-m", "--mainboardVersion", "Print version of the mainboard" },
  { "-t", "--bootTimestamps", "Print AIOC boot timestamps" },
  { "-l", "--logTimeStampsToFile", "Log AIOC boot timestamps to file" },
  { 0, 0, 0 } };

void cbc_diagnostic_print_help()
{
  printf("cbc_diagnostic version "VERSION_STRING"\n");
  printf("Intel reference design version check utility (raw serial access to IOC)\n\n");
  printf("Usage:\n");
  printf("  cbc_diagnostic [OPTION...] serial_device\n");

  printf("\n"
         "Help Options:\n"
         "  -h, --help                    Show help\n"
         "\n"
         "Application Options:\n"
         "");

  // extract longest argument
  size_t arglen = 4u;
  for (uint32_t i = 1u; cbc_diagnostic_control_options[i].long_opt != 0; ++i)
  {
    if (strlen(cbc_diagnostic_control_options[i].long_opt) > arglen)
    {
      arglen = strlen(cbc_diagnostic_control_options[i].long_opt);
    }
  }
  for (uint32_t i = 1u; cbc_diagnostic_control_options[i].long_opt != 0; ++i)
  {
    if (cbc_diagnostic_control_options[i].short_opt != 0)
    {
      printf("  %s, %s%*s %s\n", cbc_diagnostic_control_options[i].short_opt,
             cbc_diagnostic_control_options[i].long_opt,
             (int) (arglen - strlen(cbc_diagnostic_control_options[i].long_opt)), "",
             cbc_diagnostic_control_options[i].help_text);
    }
    else
    {
      printf("      %s%*s %s\n", cbc_diagnostic_control_options[i].long_opt,
             (int) (arglen - strlen(cbc_diagnostic_control_options[i].long_opt)), "",
             cbc_diagnostic_control_options[i].help_text);
    }
  }
}

/**
 * @brief check if given argument matches option with given index
 */
int cbc_diagnostic_check_option(const char * const argument, uint32_t index)
{
  if (argument != 0)
  {
    return ((strncmp(argument, cbc_diagnostic_control_options[index].long_opt, strlen(argument)) == 0)
        || (strncmp(argument, cbc_diagnostic_control_options[index].short_opt, strlen(argument)) == 0));
  }
  else
  {
    return 0;
  }
}

int cbc_diagnostic_check_log_option(CbcDiagnosticControlOptions * options, char * const argument)
{
  if (argument != 0)
  {
    uint8_t arg_pos;
    uint8_t arg_len = (uint8_t) strlen(argument);
    uint8_t long_opt_len = (uint8_t) strlen(cbc_diagnostic_control_options[6u].long_opt);

    if (strncmp(argument, cbc_diagnostic_control_options[6u].long_opt, long_opt_len) == 0)
      arg_pos = (uint8_t) strlen(cbc_diagnostic_control_options[6u].long_opt);
    else if (strncmp(argument, cbc_diagnostic_control_options[6u].short_opt, 2) == 0)
      arg_pos = 2;
    else
      return 0; // not log file option

    if (arg_len == 2 || arg_len == long_opt_len)
    {
      options->boot_timestamps_flag = 2u;
      return 1; // use default log file name
    }

    if (argument[arg_pos] != '=')
      return 0;

    options->boot_timestamps_flag = 2u;
    if (strlen(argument) == ++arg_pos)
    {
      printf("Missing file parameter using default\n");
      return 1;
    }

    options->log_file_name = &argument[arg_pos];

    return 1;
  }
  else
  {
    return 0;
  }
}

int32_t cbc_diagnostic_parse_option(CbcDiagnosticControlOptions * options, int argc, char *argv[])
{
  int result = 0;

  if (options == 0)
  {
    result = -3;
  }

  for (int i = 1; (i < argc) && (result == 0); ++i)
  {
    // help
    if (cbc_diagnostic_check_option(argv[i], 0u))
    {
      cbc_diagnostic_print_help();
      result = -1;
    }
    // verbose
    else if (cbc_diagnostic_check_option(argv[i], 1u))
    {
      options->verbose_flag = 1u;
    }
    // bootloader
    else if (cbc_diagnostic_check_option(argv[i], 2u))
    {
      options->output_selection |= eIasPrintFlagBootloaderVersion;
    }
    // firmware
    else if (cbc_diagnostic_check_option(argv[i], 3u))
    {
      options->output_selection |= eIasPrintFlagFirmwareVersion;
    }
    // mainboard
    else if (cbc_diagnostic_check_option(argv[i], 4u))
    {
      options->output_selection |= eIasPrintFlagMainboardVersion;
    }
    else if (cbc_diagnostic_check_option(argv[i], 5u))
    {
      options->boot_timestamps_flag = 1u;
    }
    else if (cbc_diagnostic_check_log_option(options, argv[i]))
    {
      printf("Parsed log file option correctly\n");
    }
    else
    {
      if (i < (argc - 1)) // the last parameter is the device name and not an option
      {
        printf("Unknown parameter: %s\n\n", argv[i]);
        cbc_diagnostic_print_help();
        result = -1;
      }
    }
  }

  return result;
}

