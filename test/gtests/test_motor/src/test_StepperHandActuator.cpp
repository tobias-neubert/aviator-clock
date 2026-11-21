#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "platform/motor/StepperHandActuator.h"
#include "MockStepperController.h"
#include "MockHomingStrategy.h"

using namespace aviator::platform::motor;
using namespace aviator::test;
using ::testing::NiceMock;
using ::testing::Return;

class StepperHandActuatorTest : public ::testing::Test {
protected:
    NiceMock<MockStepperController> controller;
    NiceMock<MockHomingStrategy> homingStrategy;
    StepperActuatorConfig config{
        .fullStepsPerRevolution = 1600,
        .normalSpeedDegreesPerSecond = 360.0,
        .normalAccelerationDegreesPerSecondSquared = 1000.0,
        .jumpSpeedDegreesPerSecond = 720.0,
        .enablePin = 33,
        .enablePinActiveLow = true,
        .autoDisableWhenIdle = true
    };
    StepperHandActuator actuator{controller, homingStrategy, config};
};

TEST_F(StepperHandActuatorTest, InitialState_IsNotHomedAndIdle) {
    EXPECT_FALSE(actuator.isHomed());
    EXPECT_FALSE(actuator.isBusy());
    EXPECT_DOUBLE_EQ(actuator.getCurrentAngle(), 0.0);
    EXPECT_DOUBLE_EQ(actuator.getTargetAngle(), 0.0);
    EXPECT_EQ(actuator.getState(), StepperHandActuator::State::IDLE);
}

TEST_F(StepperHandActuatorTest, SetTargetAngle_WhenNotHomed_StoresTargetWithoutMoving) {
    EXPECT_CALL(controller, moveTo(testing::_)).Times(0);

    actuator.setTargetAngle(180.0);
    EXPECT_DOUBLE_EQ(actuator.getTargetAngle(), 180.0);

    // Pump update: because it's not homed, no move should be commanded
    actuator.update();
    EXPECT_FALSE(actuator.isHomed());
}

TEST_F(StepperHandActuatorTest, Home_WhenIdle_InitiatesHomingSequence) {
    EXPECT_CALL(homingStrategy, resetStrategy()).Times(1);
    EXPECT_CALL(homingStrategy, beginHoming()).WillOnce(Return(true));
    EXPECT_CALL(homingStrategy, getHomingResult()).WillRepeatedly(Return(HomingResult::IN_PROGRESS));
    EXPECT_CALL(homingStrategy, updateHoming()).WillRepeatedly(Return(HomingResult::IN_PROGRESS));

    EXPECT_TRUE(actuator.home());
    EXPECT_TRUE(actuator.isBusy());

    actuator.update();
    EXPECT_EQ(actuator.getState(), StepperHandActuator::State::HOMING_IN_PROGRESS);
    EXPECT_TRUE(actuator.isHoming());
}

TEST_F(StepperHandActuatorTest, Home_WhenAlreadyHoming_IsRejected) {
    EXPECT_CALL(homingStrategy, beginHoming()).WillOnce(Return(true));
    EXPECT_CALL(homingStrategy, getHomingResult()).WillRepeatedly(Return(HomingResult::IN_PROGRESS));

    EXPECT_TRUE(actuator.home());
    actuator.update();

    // Secondary home attempt while still homing
    EXPECT_FALSE(actuator.home());
}

TEST_F(StepperHandActuatorTest, Home_WhenHomingFails_EntersHomingFailedState) {
    EXPECT_CALL(homingStrategy, beginHoming()).WillOnce(Return(false));

    EXPECT_TRUE(actuator.home());
    actuator.update();

    EXPECT_EQ(actuator.getState(), StepperHandActuator::State::HOMING_FAILED);
    EXPECT_TRUE(actuator.isHomingFailed());
    EXPECT_FALSE(actuator.isHomed());
    EXPECT_FALSE(actuator.isBusy());
}

TEST_F(StepperHandActuatorTest, HomingSuccessful_SetsHomedAndZeroPosition) {
    EXPECT_CALL(homingStrategy, beginHoming()).WillOnce(Return(true));
    EXPECT_CALL(homingStrategy, getHomingResult()).WillOnce(Return(HomingResult::IN_PROGRESS));

    actuator.home();
    actuator.update();

    // Homing strategy reports completion
    EXPECT_CALL(homingStrategy, updateHoming()).WillOnce(Return(HomingResult::SUCCESS));
    EXPECT_CALL(controller, setCurrentPosition(0)).Times(1);

    actuator.update();
    EXPECT_TRUE(actuator.isHomed());
    EXPECT_FALSE(actuator.isBusy());
    EXPECT_EQ(actuator.getState(), StepperHandActuator::State::IDLE);
    EXPECT_DOUBLE_EQ(actuator.getCurrentAngle(), 0.0);
}

TEST_F(StepperHandActuatorTest, MoveToAngle_WhenHomed_CalculatesStepsAndTransitionsToMoving) {
    // 1. Home actuator
    EXPECT_CALL(homingStrategy, beginHoming()).WillOnce(Return(true));
    EXPECT_CALL(homingStrategy, getHomingResult()).WillOnce(Return(HomingResult::SUCCESS));
    actuator.home();
    actuator.update();
    ASSERT_TRUE(actuator.isHomed());

    // 2. Command move to 90 degrees (1600 steps * 90 / 360 = 400 steps)
    EXPECT_CALL(controller, moveTo(400)).Times(1);
    EXPECT_CALL(controller, distanceToGo()).WillRepeatedly(Return(400));
    EXPECT_CALL(controller, getCurrentPosition()).WillRepeatedly(Return(0));

    actuator.setTargetAngle(90.0);
    EXPECT_TRUE(actuator.isBusy());

    actuator.update();
    EXPECT_EQ(actuator.getState(), StepperHandActuator::State::MOVING);

    // 3. Complete move
    EXPECT_CALL(controller, run()).WillRepeatedly(Return(false));
    EXPECT_CALL(controller, distanceToGo()).WillRepeatedly(Return(0));
    EXPECT_CALL(controller, getCurrentPosition()).WillRepeatedly(Return(400));

    actuator.update();
    EXPECT_EQ(actuator.getState(), StepperHandActuator::State::IDLE);
    EXPECT_FALSE(actuator.isBusy());
    EXPECT_DOUBLE_EQ(actuator.getCurrentAngle(), 90.0);
}

TEST_F(StepperHandActuatorTest, WrapAroundJump_AppliesHighSpeedMultiplier) {
    // 1. Home actuator
    EXPECT_CALL(homingStrategy, beginHoming()).WillOnce(Return(true));
    EXPECT_CALL(homingStrategy, getHomingResult()).WillOnce(Return(HomingResult::SUCCESS));
    actuator.home();
    actuator.update();

    // 2. Wrap-around move across 360 degrees: speed should be boosted to jump speed (720 deg/s = 3200 steps/s)
    EXPECT_CALL(controller, setMaxSpeed(3200.0f)).Times(1);
    EXPECT_CALL(controller, moveTo(1600)).Times(1);
    EXPECT_CALL(controller, distanceToGo()).WillRepeatedly(Return(1600));

    actuator.setTargetAngle(360.0);
    actuator.update();
}

TEST_F(StepperHandActuatorTest, SetCurrentAngle_OverridesAngleImmediately) {
    EXPECT_CALL(controller, setCurrentPosition(0)).Times(1);

    actuator.setCurrentAngle(0.0);
    EXPECT_DOUBLE_EQ(actuator.getCurrentAngle(), 0.0);

    actuator.update();
    EXPECT_DOUBLE_EQ(actuator.getCurrentAngle(), 0.0);
}

TEST_F(StepperHandActuatorTest, Stop_StopsControllerAndCancelsHoming) {
    EXPECT_CALL(homingStrategy, beginHoming()).WillOnce(Return(true));
    EXPECT_CALL(homingStrategy, getHomingResult()).WillOnce(Return(HomingResult::IN_PROGRESS));
    actuator.home();
    actuator.update();

    EXPECT_CALL(homingStrategy, cancelHoming()).Times(1);
    EXPECT_CALL(controller, stop()).Times(1);
    actuator.stop();
    actuator.update();
}
