/*
 @COPYRIGHT_TAG@
 */
/**
 * @file
 */

#include <gtest/gtest.h>

#include "../../../inc/cbc_diagnostic_control/cbc_diagnostic_control_frame_handler.h"
#include "../../../inc/cbc_diagnostic_control/cbc_diagnostic_control_output_flag.h"

#ifdef __cplusplus
extern "C" {
#endif



#ifdef __cplusplus
}
#endif

class CbcDiagnosticControlFrameHandler : public ::testing::Test
{
  public:
    CbcDiagnosticControlFrameHandler()
        : ::testing::Test(),
          frame_granularity(0u)
    {
    }
    ;
    virtual ~CbcDiagnosticControlFrameHandler()
    {
    }
    ;

    int pipefd[2];
    uint8_t frame_granularity;

  protected:

    // Sets up the test fixture.
    virtual void SetUp()
    {
      int res = pipe(pipefd);
      ASSERT_EQ(0, res);
    }

    // Tears down the test fixture.
    virtual void TearDown()
    {
      close(pipefd[0]); // read end
      close(pipefd[1]); // write end
    }
};

TEST_F(CbcDiagnosticControlFrameHandler, test_cbc_diagnostic_control_send_request)
{
  ASSERT_EQ(-1, cbc_diagnostic_send_request(0, 0, 0));
  //cbc_diagnostic_send_request(uint8_t verbose, uint8_t output_selection, uint8_t boot_timestamps_flag)

  ASSERT_EQ(0, cbc_diagnostic_send_request(-1, eIasPrintFlagNone, eIasTimestampsNone));

  ASSERT_EQ(0, cbc_diagnostic_send_request(0, eIasPrintFlagBootloaderVersion, eIasTimestampsNone));

  ASSERT_EQ(0, cbc_diagnostic_send_request(0, eIasPrintFlagAll, eIasTimestampsNone));

  ASSERT_EQ(0, cbc_diagnostic_send_request(0, 0, eIasTimestampsShow));

  ASSERT_EQ(0, cbc_diagnostic_send_request(0, eIasPrintFlagAll, eIasTimeStampsLog));
}

TEST_F(CbcDiagnosticControlFrameHandler, test_cbc_diagnostic_control_receive_answer)
{
#if 0
  uint8_t buf[128u];
  memset(buf, 0, sizeof(buf));

  buf[3u] = 0x01u;
  buf[4u] = 0x00u;
  buf[5u] = 0x00u;
  buf[6u] = 0x00u;

  buf[7u] = 0x02u;
  buf[8u] = 0x00u;
  buf[9u] = 0x00u;
  buf[10u] = 0x00u;

  buf[11u] = 0x03u;
  buf[12u] = 0x00u;
  buf[13u] = 0x00u;
  buf[14u] = 0x00u;

  buf[15u] = 0x04u;
  buf[16u] = 0x00u;
  buf[17u] = 0x00u;
  buf[18u] = 0x00u;

  buf[19u] = 0x05u;
  buf[20u] = 0x00u;
  buf[21u] = 0x00u;
  buf[22u] = 0x00u;

  buf[23u] = 0x06u;
  buf[24u] = 0x00u;
  buf[25u] = 0x00u;
  buf[26u] = 0x00u;

  buf[27u] = 0x07u; /* padding bytes??? */

  uint8_t frame_size = cbc_frame_helper_get_frame_size_for_payload_size(25u);
  (void) cbc_frame_helper_write_frame_header(e_ias_channel_diagnosis, 3u, buf, frame_size);
  (void) cbc_frame_helper_finalize_frame(buf, frame_size);

  cbc_diagnostic_control_payload(buf, frame_size);

  cbc_diagnostic_control_version(&buf[3u], frame_size, eIasPrintFlagAll);
  cbc_diagnostic_control_version(&buf[3u], frame_size, eIasPrintFlagBootloaderVersion);
  cbc_diagnostic_control_version(&buf[3u], frame_size, eIasPrintFlagFirmwareVersion);
  cbc_diagnostic_control_version(&buf[3u], frame_size, eIasPrintFlagMainboardVersion);
  cbc_diagnostic_control_version(&buf[3u], frame_size, eIasPrintFlagNone);
  cbc_diagnostic_control_version(&buf[3u], 2u, eIasPrintFlagAll); /* just pretend to not have enough data */

  /* valid frame */
  uint8_t buffer[] =
  { 0x05u, 0x18u, 0x23u, 0x05u, 0x03u, 0x00u, 0x00u, 0x00u, 0x02u, 0x00u, 0x00u, 0x00u, 0x07u, 0x21u, 0x9eu, 0x00u,
    0x03u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x0fu, 0x00u, 0x00u, 0x00u, 0x0bu, 0x00u, 0x00u, 0xd3u, };
  write(pipefd[1], buffer, sizeof(buffer));
  ASSERT_EQ(0, cbc_diagnostic_control_receive_answer(pipefd[0u], eIasPrintFlagAll, 1));
  ASSERT_EQ(-1, cbc_diagnostic_control_receive_answer(-1, eIasPrintFlagAll, 1));
#endif
}

