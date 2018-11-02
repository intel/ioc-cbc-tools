/* Copyright (C) 2018 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @file
 *
 * Retrieves fw, bootloader version and boot timestamps from AIOC
 * 
 */

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include <inttypes.h>

#include "../../inc/cbc_diagnostic_control/cbc_diagnostic_control_output_flag.h"

const int32_t poll_timeout = 200;

uint64_t abl_start_timestamp = 0;

#define IAS_TIMESTAMP_SIZE (8U)
#define MAX_TOTAL_FRAME_SIZE (96U)

#define CBC_DIAG_DEVICE "/dev/cbc-diagnosis"
#define CBC_DLT_DEVICE "/dev/cbc-dlt"

#define CBC_DEBUG_VERSION_REQUEST (4)

#ifdef NDEBUG
#define DEBUG_PRINT(fmt, args...)   /* Don't do anything in release builds */
#else
#define DEBUG_PRINT(fmt, args...)  fprintf(stderr, fmt, ##args)
#endif


struct pollfd pollTable[2];

int cbc_init_devices()
{
  int fd = 0;
  fd = open(CBC_DIAG_DEVICE, O_RDWR | O_NOCTTY | O_NDELAY);
  if (fd < 0)
  {
    printf("Unable to open cbc-diagnostic device\n");
    return -1;
  }

  pollTable[0].fd = fd;
  pollTable[0].events = POLLIN;

  fd = open(CBC_DLT_DEVICE, O_RDWR | O_NOCTTY | O_NDELAY);
  if (fd < 0)
  {
    printf("Unable to open cbc-dlt device\n");
    return -1;
  }

  pollTable[1].fd = fd;
  pollTable[1].events = POLLIN;

  return 0;
}

void cbc_close_devices()
{
  if (pollTable[0].fd > 0)
  {
    close(pollTable[0].fd);
    pollTable[0].fd = -1;
  }

  if (pollTable[1].fd > 0)
  {
    close(pollTable[1].fd);
    pollTable[1].fd = -1;
  }
}

void copy_timestamp_from_buffer(uint8_t *buffer, uint64_t* timestamp)
{
  uint8_t i = 0U;

  *timestamp = 0U;

  for (i = 0U; i < IAS_TIMESTAMP_SIZE; ++i)
  {
    *timestamp |= ((uint64_t) buffer[i]) << (8U * i);
  } /* for */

  printf("TS  %lu\n", *timestamp);
}

void cbc_diagnostic_print_payload(uint8_t * buffer, size_t buflen)
{
  for (size_t i = 0; i < buflen;)
  {
    for (size_t j = 0; i < buflen && j < 8; ++i, ++j)
    {
      DEBUG_PRINT("%02x ", buffer[i]);
    }
    DEBUG_PRINT("\n");
  }
}

void cbc_diagnostic_print_version(uint8_t *buffer, size_t buflen, uint8_t output_flags)
{
  /* buflen:
   * 3 * uint32_t : bootloader version
   * 3 * uint32_t : firmware version
   * 1 * uint8_t  : mainboard version
   */

  if (buflen > 23) /* 6*4 + 1 */
  {
    /* versions are stored as follows:
     * bootloader major, bootloader minor, bootloader revision (all uint32_t values)
     * firmware major, firmware minor, firmware revision (all uint32_t values)
     * mainboard_revision (uint8_t value)
     */
    uint32_t * version = (uint32_t *) buffer; /* this only works with little endian! */

    if ((output_flags & eIasPrintFlagBootloaderVersion))
    {
      printf("Bootloader version: %u.%u.%u\n", version[0], version[1], version[2]);
    }

    if ((output_flags & eIasPrintFlagFirmwareVersion) != 0)
    {
      printf("Firmware version: %u.%u.%u\n", version[3], version[4], version[5]);
    }

    if ((output_flags & eIasPrintFlagMainboardVersion) != 0)
    {
      printf("Mainboard version: %hu\n", buffer[24]);
    }
  }
}

void cbc_parse_timestamp(uint8_t *buffer, FILE* file)
{
  uint64_t timestamp;

  uint8_t reason_code = buffer[0];

  copy_timestamp_from_buffer(++buffer, &timestamp);

  if (reason_code == 2)
    abl_start_timestamp = timestamp;

  timestamp -= abl_start_timestamp;

  printf("BTMCBC %d %" PRIu64 "\n", reason_code, timestamp);
  if (file)
  {
    fprintf(file, "BTMCBC %d %" PRIu64 "\n", reason_code, timestamp);
  }
}

int cbc_diagnostic_send_request(uint8_t verbose, uint8_t output_selection, uint8_t boot_timestamps_flag)
{
  uint8_t success = -1;

  if (verbose)
    DEBUG_PRINT("cbc_diagnostic_send_request os %u bs %u fd 0 %d fd 1 %d\n", 
    		output_selection, boot_timestamps_flag,
         	pollTable[0].fd, pollTable[1].fd);

  if (output_selection != eIasPrintFlagNone && pollTable[0].fd > 0)
  {
    uint8_t frame_buffer[32u]; /* assert(frame_size < sizeof(frame_buffer) */
    frame_buffer[0U] = CBC_DEBUG_VERSION_REQUEST;

    if (verbose)
      DEBUG_PRINT("output_selection flag %u\n", output_selection);

    if (verbose)
    {
      DEBUG_PRINT("Sending out version request:\n");
      cbc_diagnostic_print_payload(frame_buffer, 1);
    }

    ssize_t const bytes_written = write(pollTable[0].fd, frame_buffer, 1);
    if (bytes_written != 1)
    {
      printf("Error sending data. Written bytes: %zi expected: %i\n", bytes_written, 1);
      return -1;
    }

    if (verbose)
    {
      DEBUG_PRINT("Diag. data successfully sent\n");
    }
    success = 0;
  }

  if (boot_timestamps_flag != eIasTimestampsNone && pollTable[1].fd > 0)
  {

    uint8_t frame_buffer[32u]; /* assert(frame_size < sizeof(frame_buffer) */
    frame_buffer[0U] = 255;

    if (verbose)
      DEBUG_PRINT("boot_timestamps_flag %u\n", boot_timestamps_flag);

    if (output_selection != eIasPrintFlagNone)
      usleep(100000);

    if (verbose)
    {
      DEBUG_PRINT("Sending out timestamps request:\n");
      cbc_diagnostic_print_payload(frame_buffer, 1);
    }

    ssize_t const bytes_written = write(pollTable[1].fd, frame_buffer, 1);
    if (bytes_written != 1)
    {
      printf("Error sending data. Written bytes: %zi expected: %i\n", bytes_written, 1);
      return -1;
    }

    if (verbose)
    {
      DEBUG_PRINT("Dlt Data successfully sent\n");
    }
    success = 0;
  }

  return success;
}

int cbc_diagnostic_receive_answer(uint8_t verbose, uint8_t output_flags, uint8_t boot_timestamps_flag,
                                  const char* log_file)
{
  pollTable[0].revents = 0;
  pollTable[1].revents = 0;

  FILE * file = NULL;

  if (boot_timestamps_flag == 2u)
  {
    file = fopen(log_file, "w");
    if (file == NULL)
    {
	printf("log file opened error\n");
	return -1;
    }
  }

  int32_t ret = poll(pollTable, 2, poll_timeout);

  if (ret < 0)
  {
    printf("Failed to poll serial device:\n");
    fclose(file);
    return -1;
  }
  else if (ret > 0)
  {
    uint8_t buffer[MAX_TOTAL_FRAME_SIZE];
    ssize_t read_chars = 0;

    uint8_t buffer2[MAX_TOTAL_FRAME_SIZE];
    ssize_t read_chars2 = 0;

    uint8_t * bptr = NULL;

    if (output_flags != eIasPrintFlagNone && (pollTable[0].revents & POLLIN))
    {
      read_chars = read(pollTable[0].fd, buffer, sizeof(buffer));
      if (verbose)
      {
        DEBUG_PRINT("diag received data sz  %zu\n", read_chars);
        cbc_diagnostic_print_payload(buffer, (size_t) (read_chars));
      }

      bptr = buffer;
      cbc_diagnostic_print_version(++bptr, read_chars, output_flags);
      usleep(100000);
    }

    if (boot_timestamps_flag != eIasTimestampsNone && (pollTable[1].revents & POLLIN))
    {
      do
      {
        read_chars2 = read(pollTable[1].fd, buffer2, sizeof(buffer2));
        if (read_chars2 < 0)
          printf("Read all frames? %d\n", errno);
        else
        {
          if (verbose)
	  {
            DEBUG_PRINT("dlt received data sz  %zu\n", read_chars2);
            cbc_diagnostic_print_payload(buffer2, (size_t) (read_chars2));
          }

          bptr = buffer2;
          cbc_parse_timestamp(++bptr, file);
        }
        usleep(100000);

      }
      while (read_chars2 > 0);
    }
  }

  fclose(file);

  return 0;
}

