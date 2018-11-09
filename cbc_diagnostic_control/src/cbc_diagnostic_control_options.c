/* Copyright (C) 2018 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @file
 *
 * Retrieves fw, bootloader version and boot timestamps from AIOC
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#include <cbc_diagnostic_control_options.h>
#include <cbc_diagnostic_control_output_flag.h>

/* Original output:
 * ./vehicle_bus/cbc_diagnostic/cbc_diagnostic [OPTION...] serialDeviceName
 * Intel reference design version check utility (raw serial access)

 * Help Options:
 * -h, --help                    Show help

 * Application Options:
 * -b,        Print the bootloader version.
 * -f,        Print the firmware version.
 * -m,        Print the mainboard version.
 *
 */

void usage(void)
{
	printf("Usage:\n");
	printf("  cbc_diagnostic [OPTIONS...]\n");
	printf("Options:\n");
	printf(" -h		Show help\n");
	printf(" -v		Print verbose\n");
	printf(" -b		Print bootloader version\n");
	printf(" -f		Print firmware version\n");
	printf(" -m		Print mainboard version\n");
	printf(" -t		Print AIOC boot timestamps\n");
	printf(" -l		Log AIOC boot timestamps to file\n");
}



int32_t cbc_diagnostic_parse_option(CbcDiagnosticControlOptions *options,
				    int argc, char *argv[])
{
	int result = 0;
	int c;

	if (argc == 1)  {
		usage();
		return -1;
	}

	while ((c = getopt(argc, argv, "hvbfmtl:")) != -1)  {
		switch (c)  {
			case 'v':
				options->verbose_flag = 1u;
				break;

			case 'h':
				usage();
				return -1;

			case 'b':
				options->output_selection |=
					eIasPrintFlagBootloaderVersion;
				break;

			case 'f':
				options->output_selection |=
					eIasPrintFlagFirmwareVersion;
				break;

			case 'm':
				options->output_selection |=
					eIasPrintFlagMainboardVersion;
				break;

			case 't':
				options->boot_timestamps_flag = 1u;
				break;

			case 'l':
				options->boot_timestamps_flag = 2u;
				options->log_file_name = optarg;
				break;

			case '?':
				if (optopt == 'l')
					fprintf(stderr,
					"Option -%c requires an argument.\n",
						optopt);
				else if (isprint (optopt))
					fprintf(stderr,
					"Unknown option '-%c'.\n",
						optopt);
				else
					fprintf(stderr,
					"Unknown option character '\\x%x'.\n",
						optopt);

				usage();
				return -1;

			default:
				abort();
				return -1;

		} //End of switch
	} //End of while

	return result;
}

