#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <aviator-clock/ClockHand.h>
#include <soc/mock/MockSoc.h>

#include "StepperMotorMock.h"

using namespace ::testing;
using namespace aviator_clock;
using namespace soc::mock;
using namespace soc::api;

// Test fixture for ClockHand
class ClockHandTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    mockSoc = std::make_unique<MockSoc>();
    auto motorMockUnique = std::make_unique<NiceMock<StepperMotorMock>>();
    // Keep a raw pointer for setting test expectations
    motorMock = motorMockUnique.get();

    // Create the ClockHand, moving ownership of the mock motor into it
    clockHand = std::make_unique<ClockHand>(
        ClockHand::HandType::SECOND, *mockSoc, std::move(motorMockUnique),
        360.0, // Simplified total angle for easier math
        2400.0, 60000.0);
  }

  void TearDown() override
  {
    // Ensure task is shut down before the next test
    if (clockHand)
    {
      clockHand.reset();
    }
  }

  // Members for the test fixture
  std::unique_ptr<MockSoc> mockSoc;
  NiceMock<StepperMotorMock> *motorMock; // Raw pointer for setting expectations
  std::unique_ptr<ClockHand> clockHand;
};

TEST_F(ClockHandTest, SetupConfiguresMotor)
{
  EXPECT_CALL(*motorMock, setSpeed(2400.0));
  EXPECT_CALL(*motorMock, setAcceleration(60000.0));
  clockHand->setup();
}

TEST_F(ClockHandTest, StartPerformsHomingAndInitialization)
{
  // Set up the mock's behavior. We want it to look like homing succeeds.
  ON_CALL(*motorMock, home()).WillByDefault(Return(true));
  // Simulate that homing is needed initially, then it completes.
  EXPECT_CALL(*motorMock, needsHoming())
      .WillOnce(Return(true))
      .WillRepeatedly(Return(false));
  EXPECT_CALL(*motorMock, isBusy()).WillRepeatedly(Return(false));
  EXPECT_CALL(*motorMock, isHomingFailed()).WillRepeatedly(Return(false));

  // We expect that after homing, an initial move is commanded.
  // MockTime returns 10:20:30, so for a SECOND hand, it should move to 30.
  // 360 degrees / 60 seconds = 6 degrees per second. 30 * 6 = 180 degrees.
  EXPECT_CALL(*motorMock, moveToAbsolute(180.0)).Times(1);

  // Action: Setup and start the task
  clockHand->setup();
  clockHand->start();

  // Give the task time to run through its blocking startup sequence.
  mockSoc->getSleeper().sleep(100);
}
