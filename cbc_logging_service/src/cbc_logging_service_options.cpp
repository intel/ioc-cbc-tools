/* Copyright (C) 2018 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clasue
 *
 * @file
 *
 * This tool receives boot timestamps and dlt logging from AIOC
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <cbc_logging_service_options.h>


void usage()
{
	printf("Usage: cbc_logging <Options> [-d <log_level>]\n");
	printf("Options:\n");
	printf(" -h			Usage\n");
	printf(" -v			Print verbose\n"); /* optional parameter */
	printf(" -t			Print AIOC boot timestamps\n");
	printf(" -p			Enable default DLT local prints\n");
	printf("log files\n");
	printf(" -l	btstamps_file		Log AIOC boot timestamps to file\n");
	printf(" -c 	dlt_file 	Convert DLT file to txt file\n");
	printf(" -o 	dlt_file		Output messages to new DLT file\n");
}

int32_t cbc_logging_parse_option(CbcLoggingServiceControlOptions * options,
				int argc, char *argv[])
{
	char *dlt_log = 0;
	int c;

	opterr = 0;

	if (argc == 1)
	{
		usage();
		return -1;
	}

	options->dlt_log_level = 1;
	options->dlt_file = NULL;
	options->btstamps_file = NULL;
	while ((c = getopt(argc, argv, "vhtpl:c:o:d:")) != -1)
	{
		switch(c)
		{
			case 'v':
				options->verbose_flag = 1;
				break;

			case 'p':
				options->dlt_prints = 1;
				break;

			case 't':
				options->boot_timestamps_flag = 1;
				break;

			case 'c':
				options->dlt_file = optarg;
				options->convert = 1;
				break;

			case 'l':
				options->btstamps_file = optarg;
				break;

			case 'o':
				options->dlt_file = optarg;
				break;

			case 'd':
				dlt_log = optarg;
				break;

			case 'h':
				usage();
				return -1;

			case '?':
				{
					if (optopt == 'l' || optopt == 'o' || optopt == 'c')
					{
						fprintf(stderr, "Option -%c requires an argument.\n", optopt);
					}
					else if (isprint (optopt))
					{
						fprintf(stderr, "Unknown option '-%c'.\n", optopt);
					}
					else
					{
						fprintf(stderr, "Unknown option character '\\x%x'.\n", optopt);
					}

					usage();
					return -1;
				}
			default:
				{
					abort();
					return -1;
				}
		} //End of switch
	}//End of while

	if (dlt_log)
	{
		options->dlt_log_level = atoi(dlt_log);
	}
	else
	{
		options->dlt_log_level = 1;
	}

	return 0;
}


