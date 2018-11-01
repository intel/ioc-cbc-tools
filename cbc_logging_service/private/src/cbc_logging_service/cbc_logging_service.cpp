/* Copyright (C) 2018 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clasue
 *
 * @file
 *
 * This tool receives boot timestamps and dlt logging from AIOC
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

#include <dlt/dlt.h>

#include "../../inc/cbc_logging_service/cbc_logging_service.h"
#include "../../inc/cbc_logging_service/cbc_logging_service_options.h" 

const int32_t poll_timeout = 200;

uint64_t abl_start_timestamp = 0;

#define IAS_TIMESTAMP_SIZE (8U)
#define IAS_CBC_MAX_SERVICE_FRAME_SIZE (64U)
#define MAX_TOTAL_FRAME_SIZE (96U)

#define CBC_DLT_DEVICE "/dev/cbc-dlt"

#define CBC_DEBUG_VERSION_REQUEST (4)

#define IOC_LOG_HEADER_SIZE (sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint8_t)*2)
#define MAX_IOC_LOG_ARGUMENT_SIZE (IAS_CBC_MAX_SERVICE_FRAME_SIZE - IOC_LOG_HEADER_SIZE)

#ifdef NDEBUG
#define DEBUG_PRINT(fmt, args...)   /* Don't do anything in release builds */
#else
#define DEBUG_PRINT(fmt, args...)  fprintf(stderr, fmt, ##args)
#endif

struct pollfd pollTable[1];

int running = 0;

DltContext dltContext;

uint64_t last_timestamp;

int cbc_init_device(CbcLoggingServiceControlOptions * options)
{
  int fd = 0;

  if (options->dlt_file)
  {
    if (dlt_init_file(options->dlt_file) < 0) /* log to file */
    {
      printf("Output dlt file creation error\n");
      return -1;
    }
   }

  fd = open(CBC_DLT_DEVICE, O_RDWR | O_NOCTTY | O_NDELAY);
  
  if (fd < 0)
  {
    printf("Unable to open cbc-dlt device\n");
    return -1;
  }

  pollTable[0].fd = fd;
  pollTable[0].events = POLLIN;

  running = 1;

  (void)dlt_register_app("IVDL", "IAS CBC IOC DLT logger");
  (void)dlt_register_context(&dltContext, "_IOC", "CBC IOC DLT logger context");

  //DLT_SET_APPLICATION_LL_TS_LIMIT(DLT_LOG_VERBOSE, DLT_TRACE_STATUS_DEFAULT);

  if (options->dlt_prints == 1)
  {
    dlt_enable_local_print();
  }

  //(void) dlt_set_application_ll_ts_limit(DLT_LOG_DEFAULT, DLT_TRACE_STATUS_DEFAULT);

  return 0;
}

void cbc_close_device()
{
  if (pollTable[0].fd > 0)
  {
    close(pollTable[0].fd);
    pollTable[0].fd = -1;
  }
}

int parse_dlt_argument(DltContextData& log_local, ias_cbc_ioc_argument_type type_argument,
                                   uint8_t size_argument, uint8_t * value_argument)
{
  switch(type_argument)
  {
    case e_ias_cbc_ioc_argument_type_string:
    {
      return dlt_user_log_write_string(&log_local, reinterpret_cast<char*>(value_argument));
    }
    case e_ias_cbc_ioc_argument_type_raw:
    {
      return dlt_user_log_write_raw(&log_local, value_argument, size_argument);
    }
    case e_ias_cbc_ioc_argument_type_bool:
    {
      return dlt_user_log_write_bool(&log_local, *(reinterpret_cast<int8_t*>(value_argument)));
    }
    case e_ias_cbc_ioc_argument_type_int:
    {
      return dlt_user_log_write_int(&log_local, *(reinterpret_cast<int32_t*>(value_argument)));
    }
    case e_ias_cbc_ioc_argument_type_int8:
    {
      return dlt_user_log_write_int8(&log_local, *(reinterpret_cast<int8_t*>(value_argument)));
    }
    case e_ias_cbc_ioc_argument_type_int16:
    {
      return dlt_user_log_write_int16(&log_local, *(reinterpret_cast<int16_t*>(value_argument)));
    }
    case e_ias_cbc_ioc_argument_type_int32:
    {
      return dlt_user_log_write_int32(&log_local, *(reinterpret_cast<int32_t*>(value_argument)));
    }
    case e_ias_cbc_ioc_argument_type_uint8:
    {
      return dlt_user_log_write_uint8(&log_local, *value_argument);
    }
    case e_ias_cbc_ioc_argument_type_uint16:
    {
      return dlt_user_log_write_uint16(&log_local, *(reinterpret_cast<uint16_t*>(value_argument)));
    }
    case e_ias_cbc_ioc_argument_type_uint32:
    {
      return dlt_user_log_write_uint32(&log_local, *(reinterpret_cast<uint32_t*>(value_argument)));
    }
    default:
    {
      return -1;
    }
  } // switch
}


void send_log(const uint8_t app_id, const uint8_t context_id,
                                const uint8_t log_lvl, const uint32_t timestamp,
                                const uint8_t size_description, uint8_t const * const description,
                                const ias_cbc_ioc_argument_type type_argument1, const uint8_t size_argument1,
                                uint8_t const * const value_argument1,
                                const ias_cbc_ioc_argument_type type_argument2, const uint8_t size_argument2,
                                uint8_t const * const value_argument2,
                                const ias_cbc_ioc_argument_type type_argument3, const uint8_t size_argument3,
                                uint8_t const * const value_argument3,
                                const ias_cbc_ioc_argument_type type_argument4, const uint8_t size_argument4,
                                uint8_t const * const value_argument4)
{
  (void)app_id;
  (void)context_id;
  (void)size_description;



  DltLogLevelType dltLogLevelType = (DltLogLevelType)log_lvl;

  // Check if timestamp is not overflow
  uint64_t lowBytesLastLogTimestamp = last_timestamp & 0xFFFFFFFF;
  if(timestamp < lowBytesLastLogTimestamp)
  {
    uint64_t highBytesLastLogTimestamp  = (last_timestamp & 0xFFFFFFFF00000000) >> 32;
    highBytesLastLogTimestamp++;

    last_timestamp= (last_timestamp & (0xFFFFFFFF00000000)) | timestamp;
    last_timestamp = (last_timestamp & (0xFFFFFFFF)) | (highBytesLastLogTimestamp << 32);
  }
  else
  {
    last_timestamp = (last_timestamp & (0xFFFFFFFF00000000)) | timestamp;
  }

  if(e_ias_cbc_ioc_argument_not_use != type_argument1 && e_ias_cbc_ioc_argument_not_use != type_argument2 &&
     e_ias_cbc_ioc_argument_not_use != type_argument3 && e_ias_cbc_ioc_argument_not_use != type_argument4)
  {

      DLT_LOG(dltContext, dltLogLevelType, DLT_STRING((const char *)description),
              DLT_STRING("["), DLT_INT64(last_timestamp), DLT_STRING("]"),
              parse_dlt_argument(log_local, type_argument1, size_argument1, const_cast<uint8_t*>(value_argument1)),
              parse_dlt_argument(log_local, type_argument2, size_argument2, const_cast<uint8_t*>(value_argument2)),
              parse_dlt_argument(log_local, type_argument3, size_argument3, const_cast<uint8_t*>(value_argument3)),
              parse_dlt_argument(log_local, type_argument4, size_argument4, const_cast<uint8_t*>(value_argument4))
      );
  }
  else if(e_ias_cbc_ioc_argument_not_use != type_argument1 && e_ias_cbc_ioc_argument_not_use != type_argument2 && e_ias_cbc_ioc_argument_not_use != type_argument3)
  {
    DLT_LOG(dltContext,  dltLogLevelType, DLT_STRING((const char *)description),
                DLT_STRING("["), DLT_INT64(last_timestamp), DLT_STRING("]"),
                parse_dlt_argument(log_local, type_argument1, size_argument1, const_cast<uint8_t*>(value_argument1)),
                parse_dlt_argument(log_local, type_argument2, size_argument2, const_cast<uint8_t*>(value_argument2)),
                parse_dlt_argument(log_local, type_argument3, size_argument3, const_cast<uint8_t*>(value_argument3))
                );
  }
  else if(e_ias_cbc_ioc_argument_not_use != type_argument1 && e_ias_cbc_ioc_argument_not_use != type_argument2)
  {
    DLT_LOG(dltContext,  dltLogLevelType, DLT_STRING((const char *)description),
                DLT_STRING("["), DLT_INT64(last_timestamp), DLT_STRING("]"),
                parse_dlt_argument(log_local, type_argument1, size_argument1, const_cast<uint8_t*>(value_argument1)),
                parse_dlt_argument(log_local, type_argument2, size_argument2, const_cast<uint8_t*>(value_argument2))
                );
  }
  else if(e_ias_cbc_ioc_argument_not_use != type_argument1)
  {
    DLT_LOG(dltContext,  dltLogLevelType, DLT_STRING((const char *)description),
                DLT_STRING("["), DLT_INT64(last_timestamp), DLT_STRING("]"),
                parse_dlt_argument(log_local, type_argument1, size_argument1, const_cast<uint8_t*>(value_argument1))
                );
  }
  else
  {
    DLT_LOG(dltContext,  dltLogLevelType, DLT_STRING((const char *)description),
                DLT_STRING("["), DLT_INT64(last_timestamp), DLT_STRING("]")
                );
  }
}

/*! \brief Parse a send log argument (CM side)
 *
 * \param [in] payload_indexer - payload indexer
 * \param [in] payload         - payload data pointer
 * \param [in] type_argument   - argument type
 * \param [in] size_argument   - argument size
 * \param [in] value_argument  - argument value
 *
 * \return #ias_error
 */
int cbc_service_debug_receive_send_log_parse_argument(uint8_t * const payload_indexer,
                                                      uint8_t const * const payload,
                                                      const ias_cbc_ioc_argument_type type_argument,
                                                      uint8_t * const size_argument,
                                                      uint8_t * const value_argument
                                                                  )
{
  int return_value = 0;
  uint8_t i        = 0U;

  /* check argument type */
  switch((ias_cbc_ioc_argument_type) type_argument)
  {
    case e_ias_cbc_ioc_argument_not_use:
    {
      break;
    }
    case e_ias_cbc_ioc_argument_type_string:
    case e_ias_cbc_ioc_argument_type_raw:
    {
      *size_argument = payload[(*payload_indexer)++];
      if(MAX_IOC_LOG_ARGUMENT_SIZE < *size_argument)
      {
        return_value = -1;
        break;
      }
      for (i = 0U; i < *size_argument; i++)
      {
        value_argument[i] = payload[(*payload_indexer)++];
      }
      break;
    }
    case e_ias_cbc_ioc_argument_type_bool:
    {
      if(MAX_IOC_LOG_ARGUMENT_SIZE < sizeof(uint8_t))
      {
        return_value = -1;
        break;
      }
      for (i = 0U; i < sizeof(uint8_t); i++)
      {
        value_argument[i] = payload[(*payload_indexer)++];
      }
      break;
    }
    case e_ias_cbc_ioc_argument_type_int:
    {
      if(MAX_IOC_LOG_ARGUMENT_SIZE < sizeof(uint32_t))
      {
        return_value = -1;
        break;
      }
      for (i = 0U; i < sizeof(uint32_t); i++)
      {
        value_argument[i] = payload[(*payload_indexer)++];
      }
      break;
    }
    case e_ias_cbc_ioc_argument_type_int8:
    {
      if(MAX_IOC_LOG_ARGUMENT_SIZE < sizeof(uint8_t))
      {
        return_value = -1;
        break;
      }
      for (i = 0U; i < sizeof(uint8_t); i++)
      {
        value_argument[i] = payload[(*payload_indexer)++];
      }
      break;
    }
    case e_ias_cbc_ioc_argument_type_int16:
    {
      if(MAX_IOC_LOG_ARGUMENT_SIZE < sizeof(uint16_t))
      {
        return_value = -1;
        break;
      }
      for (i = 0U; i < sizeof(uint16_t); i++)
      {
        value_argument[i] = payload[(*payload_indexer)++];
      }
      break;
    }
    case e_ias_cbc_ioc_argument_type_int32:
    {
      if(MAX_IOC_LOG_ARGUMENT_SIZE < sizeof(uint32_t))
      {
        return_value = -1;
        break;
      }
      for (i = 0U; i < sizeof(uint32_t); i++)
      {
        value_argument[i] = payload[(*payload_indexer)++];
      }
      break;
    }
    case e_ias_cbc_ioc_argument_type_uint8:
    {
      if(MAX_IOC_LOG_ARGUMENT_SIZE < sizeof(uint8_t))
      {
        return_value = -1;
        break;
      }
      for (i = 0U; i < sizeof(uint32_t); i++)
      {
        value_argument[i] = payload[(*payload_indexer)++];
      }
      break;
    }
    case e_ias_cbc_ioc_argument_type_uint16:
    {
      if(MAX_IOC_LOG_ARGUMENT_SIZE < sizeof(uint16_t))
      {
        return_value = -1;
        break;
      }
      for (i = 0U; i < sizeof(uint16_t); i++)
      {
        value_argument[i] = payload[(*payload_indexer)++];
      }
      break;
    }
    case e_ias_cbc_ioc_argument_type_uint32:
    {
      if(MAX_IOC_LOG_ARGUMENT_SIZE < sizeof(uint32_t))
      {
        return_value = -1;
        break;
      }
      for (i = 0U; i < sizeof(uint32_t); i++)
      {
        value_argument[i] = payload[(*payload_indexer)++];
      }
      break;
    }
    default:
    {
      return_value = -1;
      break;
    }
  } /* switch */
  return return_value;
} /* cbc_service_debug_receive_send_log_par */


/*! \brief Processes a send log request (CM side)
 *
 * \param [in] length  - length of payload in bytes
 * \param [in] payload - payload data pointer
 *
 * \return #ias_error
 */
int cbc_service_debug_receive_send_log(const uint8_t length, const uint8_t * const payload)
{
  uint32_t timestamp                                 = 0U;
  uint8_t * timestamp_bytes                          = (uint8_t*)&timestamp;
  int return_value                                    = 0;
  uint8_t payload_indexer                            = 0U;
  uint8_t i                                          = 0U;
  uint8_t app_id                                     = 0U;
  uint8_t context_id                                 = 0U;
  uint8_t log_lvl                                    = 0U;
  uint8_t size_description                           = 0U;
  uint8_t description[MAX_IOC_LOG_ARGUMENT_SIZE]     = {0U};
  ias_cbc_ioc_argument_type type_argument1           = e_ias_cbc_ioc_argument_not_use;
  uint8_t size_argument1                             = 0U;
  uint8_t value_argument1[MAX_IOC_LOG_ARGUMENT_SIZE] = {0U};
  ias_cbc_ioc_argument_type type_argument2           = e_ias_cbc_ioc_argument_not_use;
  uint8_t size_argument2                             = 0U;
  uint8_t value_argument2[MAX_IOC_LOG_ARGUMENT_SIZE] = {0U};
  ias_cbc_ioc_argument_type type_argument3           = e_ias_cbc_ioc_argument_not_use;
  uint8_t size_argument3                             = 0U;
  uint8_t value_argument3[MAX_IOC_LOG_ARGUMENT_SIZE] = {0U};
  ias_cbc_ioc_argument_type type_argument4           = e_ias_cbc_ioc_argument_not_use;
  uint8_t size_argument4                             = 0U;
  uint8_t value_argument4[MAX_IOC_LOG_ARGUMENT_SIZE] = {0U};

  /* check input parameters */
  if( (NULL == payload) || (length <= 0))
  {
    return -1;
  } /* if */

  /* check if payload length is correct */
  if( length < IOC_LOG_HEADER_SIZE)
  {
    return -1;
  } /* if */

  /* get App ID */
  app_id = payload[payload_indexer++];

  /* get context ID */
  context_id = payload[payload_indexer++];

  /* get log level */
  log_lvl = payload[payload_indexer++];

  /* get timestamp */
  for(i = 0U; i < sizeof(timestamp); i++)
  {
    timestamp_bytes[i] = payload[payload_indexer++];
  } /* for */

  /* get argument types */
  type_argument1 = (ias_cbc_ioc_argument_type) (payload[payload_indexer] & 0x0F);
  type_argument2 = (ias_cbc_ioc_argument_type) ((payload[payload_indexer++] & 0xF0) >> 4);
  type_argument3 = (ias_cbc_ioc_argument_type) (payload[payload_indexer] & 0x0F);
  type_argument4 = (ias_cbc_ioc_argument_type) ((payload[payload_indexer++] & 0xF0) >> 4);

  /* get description size */
  size_description = payload[payload_indexer++];

  if(MAX_IOC_LOG_ARGUMENT_SIZE < size_description)
  {
    return -1;
  }

  /* get description */
  for (i = 0U; i < size_description; i++)
  {
    description[i] = payload[payload_indexer++];
  } /* for */

  return_value = cbc_service_debug_receive_send_log_parse_argument(&payload_indexer, payload,
                                                                   type_argument1, &size_argument1, value_argument1);
  if(0 != return_value)
  {

    return return_value;
  } /* if */

  return_value = cbc_service_debug_receive_send_log_parse_argument(&payload_indexer, payload,
                                                                   type_argument2, &size_argument2, value_argument2);
  if(0 != return_value)
  {
    return return_value;
  } /* if */

  return_value = cbc_service_debug_receive_send_log_parse_argument(&payload_indexer, payload,
                                                                   type_argument3, &size_argument3, value_argument3);
  if(0 != return_value)
  {
    return return_value;
  } /* if */

  return_value = cbc_service_debug_receive_send_log_parse_argument(&payload_indexer, payload,
                                                                   type_argument4, &size_argument4, value_argument4);
  if(0 != return_value)
  {
    return return_value;
  } /* if */

  send_log(app_id, context_id, log_lvl, timestamp,
                       size_description, description,
                       type_argument1, size_argument1, value_argument1,
                       type_argument2, size_argument2, value_argument2,
                       type_argument3, size_argument3, value_argument3,
                       type_argument4, size_argument4, value_argument4
                      );

  return return_value;
} /* cbc_service_debug_receive_send_log */


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

int cbc_parse_timestamp(uint8_t *buffer, char* file)
{
  uint64_t timestamp;

  FILE *fp = NULL;

  uint8_t reason_code = buffer[0];

  copy_timestamp_from_buffer(++buffer, &timestamp);

  if (reason_code == 2)
    abl_start_timestamp = timestamp;

  timestamp -= abl_start_timestamp;

  printf("BTMCBC %d %" PRIu64 "\n", reason_code, timestamp);
  if (file)
  {
    fp = fopen(file, "a+");
    if (fp == NULL)
    {
      printf("file open failed\n");
      fclose(fp);
      return -1;
    }
    else 
    {
      fprintf(fp, "BTMCBC %d %" PRIu64 "\n", reason_code, timestamp);
      fclose(fp);
    }
  }
  
}

int parse_response(size_t buflen, uint8_t* buffer, char * file)
{
  switch (buffer[0]){
    case 1:
      printf("timestamp \n");
      cbc_parse_timestamp(++buffer, file);
      break;
    case 2:
      printf("log\n");
      cbc_service_debug_receive_send_log((uint8_t)buflen-1, ++buffer);
      break;
    default:
      printf("Unhandled type\n");
  }
  return 0;
}

int run_logging_service(CbcLoggingServiceControlOptions* options)
{
  uint8_t const payload_size = 1u;
  uint8_t success = -1;
  uint8_t reset[] = { 255 }; // 255 = channel reset
  
  int pingSent = 0;
  uint8_t ping[] = { 3, 28 }; // 3 = svc trigger test interface ; 28 = ping
 
  struct timespec start;
  (void)clock_gettime(CLOCK_MONOTONIC, &start);

  ssize_t bytes_written = write(pollTable[0].fd, reset, 1);
  if (bytes_written != 1)
  {
    printf("Error sending data. Written bytes: %zi expected: %i\n", bytes_written, 1);
    return -1;
  }
 
  while (running)
  {
    uint8_t buffer[MAX_TOTAL_FRAME_SIZE];
    ssize_t read_chars = 0;
    read_chars = read(pollTable[0].fd, buffer, sizeof(buffer));
    
    if (read_chars < 0 && (EAGAIN != errno))
      printf("Error reading CBC device %d\n", errno);
    else if (read_chars > 0)
    {
      if (options->verbose_flag == 1)
      {
        DEBUG_PRINT("diag received data sz  %zu\n", read_chars);
        cbc_diagnostic_print_payload(buffer, (size_t) (read_chars));
      }

      parse_response(read_chars, buffer, options->btstamps_file);
    }

    if (!pingSent)
    {
      struct timespec now;
      (void)clock_gettime(CLOCK_MONOTONIC, &now);


      if ( (now.tv_sec - start.tv_sec) > 1.0)
      {
        bytes_written = write(pollTable[0].fd, ping, 2);
        if (bytes_written != 2)
        {
          printf("Error sending data. Written bytes: %zi expected: %i\n", bytes_written, 2);
          return -1;
        }
        pingSent = 1;
      }
    }
  }//End of while
  
  return success;
}

