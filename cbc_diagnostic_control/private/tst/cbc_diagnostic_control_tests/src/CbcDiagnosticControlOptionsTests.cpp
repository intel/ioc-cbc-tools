/*
 @COPYRIGHT_TAG@
 */
/**
 * @file
 */

#include <gtest/gtest.h>

#include "../../../inc/cbc_diagnostic_control/cbc_diagnostic_control_options.h"
#include "../../../inc/cbc_diagnostic_control/cbc_diagnostic_control_output_flag.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int cbc_diagnostic_check_option(const char * const argument, uint32_t index);

#ifdef __cplusplus
}
#endif

TEST(CbcDiagnosticControlOptions, test_cbc_diagnostic_control_check_option)
{
  char short_opt[] = "-v";
  char long_opt[] = "--verbose";

  char wrong_short_opt[] = "-w";
  char wrong_long_opt[] = "--nothing";

  ASSERT_FALSE(cbc_diagnostic_check_option(NULL, 1u));

  ASSERT_TRUE(cbc_diagnostic_check_option(short_opt, 1u));
  ASSERT_TRUE(cbc_diagnostic_check_option(long_opt, 1u));

  ASSERT_FALSE(cbc_diagnostic_check_option(wrong_short_opt, 1u));
  ASSERT_FALSE(cbc_diagnostic_check_option(wrong_long_opt, 1u));

  ASSERT_FALSE(cbc_diagnostic_check_option(short_opt, 2u));
  ASSERT_FALSE(cbc_diagnostic_check_option(long_opt, 2u));
}

TEST(CbcDiagnosticControlOptions, test_cbc_diagnostic_control_print_help)
{
  cbc_diagnostic_print_help();
}

TEST(CbcDiagnosticControlOptions, test_cbc_diagnostic_control_parse_option)
{
  ASSERT_EQ(-3, cbc_diagnostic_parse_option(0, 0, 0));

  // help requested
  const char *argv1[] =
  { "test_cbc_diagnostic_control", "-h" };

  CbcDiagnosticControlOptions options;
  ASSERT_EQ(-1, cbc_diagnostic_parse_option(&options, 2, (char ** )argv1));

  // invalid short parameter
  const char *argv2[] =
  { "test_cbc_diagnostic_control", "-q" };
  // the option "-q" is currently considered a device name
  ASSERT_EQ(0, cbc_diagnostic_parse_option(&options, 2, (char ** )argv2));


  options.output_selection = eIasPrintFlagNone;
  // bootloader
  const char *argv4[] =
  { "test_cbc_diagnostic_control", "--bootloaderVersion" };
  ASSERT_EQ(0, cbc_diagnostic_parse_option(&options, 2, (char ** )argv4));
  ASSERT_NE(0, (options.output_selection & eIasPrintFlagBootloaderVersion));
}
