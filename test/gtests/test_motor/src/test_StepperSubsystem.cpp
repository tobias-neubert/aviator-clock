#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "platform/motor/StepperSubsystem.h"
#include "MockStepperController.h"
#include "MockHomingStrategy.h"
#include <soc/mock/MockSoc.h>

using namespace aviator::platform::motor;
using namespace aviator::test;
using ::testing::NiceMock;
using ::testing::Return;

class StepperSubsystemTest : public ::testing::Test {
protected:
    soc::mock::MockSoc soc;

    NiceMock<MockStepperController> hourController;
    NiceMock<MockStepperController> minuteController;
    NiceMock<MockStepperController> secondController;

    NiceMock<MockHomingStrategy> hourHoming;
    NiceMock<MockHomingStrategy> minuteHoming;
    NiceMock<MockHomingStrategy> secondHoming;

    StepperActuatorConfig config{};

    StepperHandActuator hourActuator{hourController, hourHoming, config};
    StepperHandActuator minuteActuator{minuteController, minuteHoming, config};
    StepperHandActuator secondActuator{secondController, secondHoming, config};

    StepperSubsystem subsystem{soc, hourActuator, minuteActuator, secondActuator};
};

TEST_F(StepperSubsystemTest, Accessors_ReturnCorrectActuators) {
    EXPECT_EQ(&subsystem.getHourHand(), &hourActuator);
    EXPECT_EQ(&subsystem.getMinuteHand(), &minuteActuator);
    EXPECT_EQ(&subsystem.getSecondHand(), &secondActuator);

    EXPECT_EQ(&subsystem.getHourHandActuator(), &hourActuator);
    EXPECT_EQ(&subsystem.getMinuteHandActuator(), &minuteActuator);
    EXPECT_EQ(&subsystem.getSecondHandActuator(), &secondActuator);
}

TEST_F(StepperSubsystemTest, InitialState_NotRunningNotHomedNotBusy) {
    EXPECT_FALSE(subsystem.isRunning());
    EXPECT_FALSE(subsystem.isAllHomed());
    EXPECT_FALSE(subsystem.isBusy());
}

TEST_F(StepperSubsystemTest, StepIteration_WhenAllIdle_ReturnsFalse) {
    EXPECT_FALSE(subsystem.stepIteration());
}

TEST_F(StepperSubsystemTest, StepIteration_WhenAnyActuatorActive_ReturnsTrue) {
    EXPECT_CALL(secondHoming, beginHoming()).WillOnce(Return(true));
    EXPECT_CALL(secondHoming, getHomingResult()).WillRepeatedly(Return(HomingResult::IN_PROGRESS));
    EXPECT_CALL(secondHoming, updateHoming()).WillRepeatedly(Return(HomingResult::IN_PROGRESS));

    subsystem.getSecondHand().home();
    EXPECT_TRUE(subsystem.isBusy());

    // stepIteration updates all 3, second hand is homing so it returns true
    EXPECT_TRUE(subsystem.stepIteration());
}

TEST_F(StepperSubsystemTest, HomeAll_TriggersHomingOnAllThreeActuators) {
    EXPECT_CALL(hourHoming, beginHoming()).WillOnce(Return(true));
    EXPECT_CALL(minuteHoming, beginHoming()).WillOnce(Return(true));
    EXPECT_CALL(secondHoming, beginHoming()).WillOnce(Return(true));

    EXPECT_CALL(hourHoming, getHomingResult()).WillRepeatedly(Return(HomingResult::SUCCESS));
    EXPECT_CALL(minuteHoming, getHomingResult()).WillRepeatedly(Return(HomingResult::SUCCESS));
    EXPECT_CALL(secondHoming, getHomingResult()).WillRepeatedly(Return(HomingResult::SUCCESS));

    EXPECT_TRUE(subsystem.homeAll());
    EXPECT_TRUE(subsystem.isBusy());

    subsystem.stepIteration();
    EXPECT_TRUE(subsystem.isAllHomed());
    EXPECT_FALSE(subsystem.isBusy());
}

TEST_F(StepperSubsystemTest, StopAll_StopsAllThreeActuators) {
    EXPECT_CALL(hourController, stop()).Times(1);
    EXPECT_CALL(minuteController, stop()).Times(1);
    EXPECT_CALL(secondController, stop()).Times(1);

    subsystem.stopAll();
    subsystem.stepIteration();
}

TEST_F(StepperSubsystemTest, TaskLifecycle_StartAndStop) {
    EXPECT_FALSE(subsystem.isRunning());

    subsystem.start();
    EXPECT_TRUE(subsystem.isRunning());

    subsystem.stop();
    EXPECT_FALSE(subsystem.isRunning());
}
