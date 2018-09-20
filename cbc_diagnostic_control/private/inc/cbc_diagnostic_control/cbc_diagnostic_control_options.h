/*
 @COPYRIGHT_TAG@
 */
/**
 * @file
 */

#ifndef VEHICLEBUS_CBC_DIAGNOSTIC_OPTIONS_H
#define VEHICLEBUS_CBC_DIAGNOSTIC_OPTIONS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* handling of command-line options for cbc_diagnostic_control */
typedef struct CbcDiagnosticControlOptions
{
    uint8_t verbose_flag;
    uint8_t output_selection;
    uint8_t boot_timestamps_flag;
    char* log_file_name;
} CbcDiagnosticControlOptions;

void cbc_diagnostic_print_help();

/**
 * @return 0 on success
 * @return -1 on failure
 * @return 1 if help was requested
 */
int32_t cbc_diagnostic_parse_option(CbcDiagnosticControlOptions * options, int argc, char *argv[]);

#ifdef __cplusplus
}
#endif

#endif /* VEHICLEBUS_CBC_DIAGNOSTIC_OPTIONS_H */
