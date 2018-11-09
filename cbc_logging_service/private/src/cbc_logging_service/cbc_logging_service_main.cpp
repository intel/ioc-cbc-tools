/* Copyright (C) 2018 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clasue
 * 
 * @file
 *
 * This tool receives boot timestamps and dlt logging from AIOC
 *
 */

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <dlt/dlt.h>
#include "../../inc/cbc_logging_service/cbc_logging_service.h"
#include "../../inc/cbc_logging_service/cbc_logging_service_options.h"


int main(int argc, char *argv[])
{
  CbcLoggingServiceControlOptions options;

  int fd = 0;

  int32_t result = cbc_logging_parse_option(&options, argc, argv);

  if (result != 0)
  {
    return -2;
  }

  if (options.convert == 1)
  {
    convert_file(&options);
    return 0;
  }
 
  int const serial_result = cbc_init_device(&options);

  if (0 != serial_result)
  {
    printf("Unable to configure   with error %i\n", serial_result);
    return -1;
  }

  run_logging_service(&options);
  return 0;
}

int convert_file(CbcLoggingServiceControlOptions * options)
{
  DltFile file;
  int vflag = 0, num = 0, begin = 0, end = 0;
  char text[DLT_CONVERT_TEXTBUFSIZE];
  char file_2[]="logging.txt";
  FILE *fptr;

  dlt_file_init(&file, vflag);

  if (dlt_file_open(&file, options->dlt_file, vflag) >= DLT_RETURN_OK)
  {
    while (dlt_file_read(&file,vflag) >= DLT_RETURN_OK)
    {
    }
  }
  
  end = file.counter-1;

  fptr = fopen("logging.txt", "w+");

  if (fptr == NULL)
  {
    printf("Error\n");
    return -1;
  }

  for (num = begin; num <= end ;num++)
  {
    dlt_file_message(&file,num,vflag);

    dlt_message_header(&(file.msg),text,DLT_CONVERT_TEXTBUFSIZE,vflag);
    fprintf(fptr, "%d %s ", num, text);

    dlt_message_payload(&(file.msg),text,DLT_CONVERT_TEXTBUFSIZE,DLT_OUTPUT_ASCII,vflag);
    fprintf(fptr,"[%s]\n",text);
  }
   
   fclose(fptr);
   return 0;   
}


    
