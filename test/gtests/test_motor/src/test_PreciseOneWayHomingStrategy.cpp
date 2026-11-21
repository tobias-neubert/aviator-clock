#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "platform/motor/PreciseOneWayHomingStrategy.h"
#include "MockStepperController.h"
#include <soc/mock/MockDigitalInput.h>

using namespace aviator::platform::motor;
using namespace aviator::test;
using ::testing::NiceMock;
using ::testing::Return;

class PreciseOneWayHomingStrategyTest : public ::testing::Test {
protected:
    NiceMock<MockStepperController> controller;
    soc::mock::MockDigitalInput sensor{25, true};
    PreciseHomingConfig config{
        .homingSpeed = 200.0f,
        .homingAcceleration = 1000.0f,
        .maxHomingTravel = 3200,
        .moveDirection = 1,
        .backOffDistance = 100
    };
    PreciseOneWayHomingStrategy strategy{controller, sensor, config};
};

TEST_F(PreciseOneWayHomingStrategyTest, InitialResult_IsNotYetStarted) {
    EXPECT_EQ(strategy.getHomingResult(), HomingResult::NOT_YET_STARTED);
}

TEST_F(PreciseOneWayHomingStrategyTest, BeginHoming_WhenSensorInactive_StartsSeekingForward) {
    sensor.setActive(false);

    EXPECT_CALL(controller, setAcceleration(1000.0f)).Times(1);
    EXPECT_CALL(controller, setMaxSpeed(200.0f)).Times(1);
    EXPECT_CALL(controller, move(3200)).Times(1);

    EXPECT_TRUE(strategy.beginHoming());
    EXPECT_EQ(strategy.getHomingResult(), HomingResult::IN_PROGRESS);
}

TEST_F(PreciseOneWayHomingStrategyTest, BeginHoming_WhenSensorActive_BacksOff) {
    sensor.setActive(true);

    EXPECT_CALL(controller, setAcceleration(1000.0f)).Times(1);
    EXPECT_CALL(controller, setMaxSpeed(200.0f)).Times(1);
    EXPECT_CALL(controller, move(-100)).Times(1);

    EXPECT_TRUE(strategy.beginHoming());
    EXPECT_EQ(strategy.getHomingResult(), HomingResult::IN_PROGRESS);
}

TEST_F(PreciseOneWayHomingStrategyTest, HomingSequence_SuccessfulDetectionAndZeroing) {
    sensor.setActive(false);
    strategy.beginHoming();

    // 1. Moving towards sensor
    EXPECT_CALL(controller, run()).WillOnce(Return(true));
    EXPECT_EQ(strategy.updateHoming(), HomingResult::IN_PROGRESS);

    // 2. Sensor is triggered
    sensor.setActive(true);
    EXPECT_CALL(controller, run()).WillOnce(Return(true));
    EXPECT_CALL(controller, stop()).Times(1);
    EXPECT_EQ(strategy.updateHoming(), HomingResult::IN_PROGRESS);

    // 3. Decelerating
    EXPECT_CALL(controller, run()).WillOnce(Return(true));
    EXPECT_EQ(strategy.updateHoming(), HomingResult::IN_PROGRESS);

    // 4. Standstill reached -> zero point established
    EXPECT_CALL(controller, run()).WillOnce(Return(false));
    EXPECT_CALL(controller, setCurrentPosition(0)).Times(1);
    EXPECT_EQ(strategy.updateHoming(), HomingResult::SUCCESS);
    EXPECT_EQ(strategy.getHomingResult(), HomingResult::SUCCESS);
}

TEST_F(PreciseOneWayHomingStrategyTest, BackOffSequence_SucceedsAndResumesSeeking) {
    sensor.setActive(true);
    strategy.beginHoming();

    // Still moving off switch
    EXPECT_CALL(controller, run()).WillOnce(Return(true));
    EXPECT_EQ(strategy.updateHoming(), HomingResult::IN_PROGRESS);

    // Finished back-off move, sensor is now cleared
    sensor.setActive(false);
    EXPECT_CALL(controller, run()).WillOnce(Return(false));
    EXPECT_CALL(controller, move(3200)).Times(1);
    EXPECT_EQ(strategy.updateHoming(), HomingResult::IN_PROGRESS);
}

TEST_F(PreciseOneWayHomingStrategyTest, BackOffSequence_FailsIfSensorStillActive) {
    sensor.setActive(true);
    strategy.beginHoming();

    // Finished back-off move, but sensor remains stuck active
    EXPECT_CALL(controller, run()).WillOnce(Return(false));
    EXPECT_EQ(strategy.updateHoming(), HomingResult::FAILURE_MOTOR_ERROR);
    EXPECT_EQ(strategy.getHomingResult(), HomingResult::FAILURE_MOTOR_ERROR);
}

TEST_F(PreciseOneWayHomingStrategyTest, SeekingTimeout_WhenMaxTravelExceededWithoutSensor) {
    sensor.setActive(false);
    strategy.beginHoming();

    // Controller finished full travel distance without triggering sensor
    EXPECT_CALL(controller, run()).WillOnce(Return(false));
    EXPECT_EQ(strategy.updateHoming(), HomingResult::FAILURE_TIMEOUT);
    EXPECT_EQ(strategy.getHomingResult(), HomingResult::FAILURE_TIMEOUT);
}

TEST_F(PreciseOneWayHomingStrategyTest, CancelHoming_StopsControllerAndSetsCancelled) {
    sensor.setActive(false);
    strategy.beginHoming();

    EXPECT_CALL(controller, stop()).Times(1);
    strategy.cancelHoming();
    EXPECT_EQ(strategy.getHomingResult(), HomingResult::CANCELLED);
}
