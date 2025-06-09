#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>

// --- Mocks and Class Under Test ---
#include "StepperControllerMock.h"
#include "HomingStrategyMock.h"
#include "LoggerMock.h"
#include "stepper/accel/AccelStepperMotor.h"

// --- Using declarations ---
using namespace stepper::accel;
using namespace stepper::api;
using ::testing::_;
using ::testing::Return;

class AccelStepperMotorTest : public ::testing::Test
{
protected:
    // Mocks are now created directly as members.
    stepper::testing::MockStepperController mockStepperController;
    stepper::testing::MockHomingStrategy mockHomingStrategy;
    soc::testing::LoggerMock loggerMock;

    // The object under test.
    std::unique_ptr<AccelStepperMotor> motor;

    // SetUp is now much simpler. It defines DEFAULT behaviors using ON_CALL.
    void SetUp() override
    {
        // --- Define Default Behaviors for Mocks ---
        // can be overridden by the tests as they need to
        ON_CALL(mockHomingStrategy, beginHoming()).WillByDefault(Return(true));
        ON_CALL(mockHomingStrategy, updateHoming()).WillByDefault(Return(HomingResult::IN_PROGRESS));
        ON_CALL(mockStepperController, distanceToGo()).WillByDefault(Return(0));
        ON_CALL(mockStepperController, run()).WillByDefault(Return(false));

        // --- Instantiate the motor ---
        // The constructor calls (e.g., setMaxSpeed) are implicitly allowed
        // by gmock's default behavior and don't need to be specified here.
        motor = std::make_unique<AccelStepperMotor>(
            mockStepperController,
            200 * 16, // fullStepsPerRevolution
            mockHomingStrategy,
            loggerMock,
            10,  // enablePin
            true // enablePinActiveLow
        );
    }
};

TEST_F(AccelStepperMotorTest, InitialState_IsIdle)
{
    // act & assert
    ASSERT_EQ(motor->getState(), StepperMotorState::IDLE);
}

TEST_F(AccelStepperMotorTest, Home_WhenIdle_TransitionsToHomingInProgress)
{
    // act
    motor->home();

    // assert
    ASSERT_EQ(motor->getState(), StepperMotorState::HOMING_IN_PROGRESS);
}

TEST_F(AccelStepperMotorTest, Home_WhenBeginHomingFails_TransitionsToHomingFailed)
{
    // arrange
    EXPECT_CALL(mockHomingStrategy, beginHoming()).WillOnce(Return(false));
    
    // act
    motor->home();
    
    // assert
    ASSERT_EQ(motor->getState(), StepperMotorState::HOMING_FAILED);
}

TEST_F(AccelStepperMotorTest, Home_WhenAlreadyHoming_StaysInHomingInProgress)
{
    // arrange
    motor->home();
    ASSERT_EQ(motor->getState(), StepperMotorState::HOMING_IN_PROGRESS);

    // act
    bool result = motor->home();

    // assert
    ASSERT_FALSE(result);
    ASSERT_EQ(motor->getState(), StepperMotorState::HOMING_IN_PROGRESS);
}

TEST_F(AccelStepperMotorTest, Update_WhenHomingInProgress_AndStrategySucceeds_TransitionsToIdle)
{
    // arrange
    motor->home();
    EXPECT_CALL(mockHomingStrategy, updateHoming()).WillOnce(Return(HomingResult::SUCCESS));

    // act
    motor->update();

    // assert
    ASSERT_EQ(motor->getState(), StepperMotorState::IDLE);
}

TEST_F(AccelStepperMotorTest, Update_WhenHomingInProgress_AndStrategyFails_TransitionsToHomingFailed)
{
    // arrange
    motor->home();
    EXPECT_CALL(mockHomingStrategy, updateHoming()).WillOnce(Return(HomingResult::FAILURE_TIMEOUT));

    // act
    motor->update();

    // assert
    ASSERT_EQ(motor->getState(), StepperMotorState::HOMING_FAILED);
}

TEST_F(AccelStepperMotorTest, MoveToAbsolute_WhenNotHomed_IsRejectedAndStaysIdle)
{
    // arrange
    ASSERT_TRUE(motor->needsHoming());

    // act
    bool result = motor->moveToAbsolute(180.0);

    // assert
    ASSERT_FALSE(result);
    ASSERT_EQ(motor->getState(), StepperMotorState::IDLE);
}

TEST_F(AccelStepperMotorTest, MoveToAbsolute_WhenHomed_AndDistanceToGo_TransitionsToMoving)
{
    // arrange
    // 1. transition to be homed
    motor->home();
    ON_CALL(mockHomingStrategy, updateHoming()).WillByDefault(Return(HomingResult::SUCCESS));
    motor->update();
    ASSERT_EQ(motor->getState(), StepperMotorState::IDLE);

    // Now, set expectation for the move call.
    EXPECT_CALL(mockStepperController, distanceToGo()).WillOnce(Return(1000));

    // act
    motor->moveToAbsolute(180.0);

    // assert
    ASSERT_EQ(motor->getState(), StepperMotorState::MOVING);
}

TEST_F(AccelStepperMotorTest, Update_WhenMoving_AndMoveCompletes_TransitionsToIdle)
{
    // arrange: Get the motor into the MOVING state.
    motor->home();
    ON_CALL(mockHomingStrategy, updateHoming()).WillByDefault(Return(HomingResult::SUCCESS));
    motor->update();
    EXPECT_CALL(mockStepperController, distanceToGo()).WillOnce(Return(1000));
    motor->moveToAbsolute(180.0);
    ASSERT_EQ(motor->getState(), StepperMotorState::MOVING);

    // assume rotation has been finished (distanceToGo == 0)
    EXPECT_CALL(mockStepperController, distanceToGo()).WillOnce(Return(0));

    // act
    motor->update();

    // assert
    ASSERT_EQ(motor->getState(), StepperMotorState::IDLE);
}

TEST_F(AccelStepperMotorTest, Stop_WhenMoving_AndStopsImmediately_TransitionsToIdle)
{
    // arrange: Get the motor into the MOVING state.
    motor->home();
    ON_CALL(mockHomingStrategy, updateHoming()).WillByDefault(Return(HomingResult::SUCCESS));
    motor->update();
    EXPECT_CALL(mockStepperController, distanceToGo())
        .WillOnce(Return(1000))  // For the first call in moveToAbsolute()
        .WillOnce(Return(0));    // For immediate stopping
    motor->moveToAbsolute(180.0);
    ASSERT_EQ(motor->getState(), StepperMotorState::MOVING);

    // Set expectation for the stop call.
    EXPECT_CALL(mockStepperController, stop()).Times(1);
    // The default ON_CALL for distanceToGo (returning 0) will apply after stop().

    // act
    motor->stop();

    // assert
    ASSERT_EQ(motor->getState(), StepperMotorState::IDLE);
}