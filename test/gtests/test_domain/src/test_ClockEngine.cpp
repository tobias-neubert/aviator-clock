#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "domain/ClockEngine.h"
#include "MockHandActuator.h"

using namespace aviator::domain;
using namespace aviator::test;
using ::testing::NiceMock;
using ::testing::Return;

class ClockEngineTest : public ::testing::Test {
protected:
    NiceMock<MockHandActuator> hourHand;
    NiceMock<MockHandActuator> minuteHand;
    NiceMock<MockHandActuator> secondHand;
    DialGeometry geometry{330.0};
    ClockEngine engine{hourHand, minuteHand, secondHand, geometry};
};

TEST_F(ClockEngineTest, InitialUpdate_SetsAnglesDirectly) {
    EXPECT_CALL(hourHand, setTargetAngle(geometry.hourToDegrees(10))).Times(1);
    EXPECT_CALL(minuteHand, setTargetAngle(geometry.minuteToDegrees(30))).Times(1);
    EXPECT_CALL(secondHand, setTargetAngle(geometry.secondToDegrees(45))).Times(1);

    engine.updateTime(Time(10, 30, 45));
    EXPECT_EQ(engine.getLastTime(), Time(10, 30, 45));
}

TEST_F(ClockEngineTest, SubsequentUpdate_CalculatesTargetAngles) {
    engine.updateTime(Time(10, 30, 45));

    EXPECT_CALL(hourHand, setTargetAngle(geometry.hourToDegrees(10))).Times(1);
    EXPECT_CALL(minuteHand, setTargetAngle(geometry.minuteToDegrees(30))).Times(1);
    EXPECT_CALL(secondHand, setTargetAngle(geometry.secondToDegrees(46))).Times(1);

    engine.updateTime(Time(10, 30, 46));
    EXPECT_EQ(engine.getLastTime(), Time(10, 30, 46));
}

TEST_F(ClockEngineTest, WrapAround_CommandsFullCircleTarget) {
    engine.updateTime(Time(11, 59, 59));

    EXPECT_CALL(secondHand, setTargetAngle(DialGeometry::FULL_CIRCLE_DEGREES)).Times(1);
    EXPECT_CALL(minuteHand, setTargetAngle(DialGeometry::FULL_CIRCLE_DEGREES)).Times(1);
    EXPECT_CALL(hourHand, setTargetAngle(DialGeometry::FULL_CIRCLE_DEGREES)).Times(1);

    engine.updateTime(Time(0, 0, 0));
}

TEST_F(ClockEngineTest, Update_ResetsPosition_WhenHandAtOrAboveFullCircle) {
    engine.updateTime(Time(11, 59, 59));

    EXPECT_CALL(hourHand, getCurrentAngle()).WillRepeatedly(Return(360.0));
    EXPECT_CALL(minuteHand, getCurrentAngle()).WillRepeatedly(Return(360.0));
    EXPECT_CALL(secondHand, getCurrentAngle()).WillRepeatedly(Return(360.0));

    EXPECT_CALL(hourHand, setCurrentAngle(0.0)).Times(1);
    EXPECT_CALL(minuteHand, setCurrentAngle(0.0)).Times(1);
    EXPECT_CALL(secondHand, setCurrentAngle(0.0)).Times(1);

    engine.updateTime(Time(0, 0, 0));
}

TEST_F(ClockEngineTest, HomingAndStatusCoordination) {
    EXPECT_CALL(hourHand, home()).WillOnce(Return(true));
    EXPECT_CALL(minuteHand, home()).WillOnce(Return(true));
    EXPECT_CALL(secondHand, home()).WillOnce(Return(true));
    EXPECT_TRUE(engine.homeAll());

    EXPECT_CALL(hourHand, isHomed()).WillOnce(Return(true));
    EXPECT_CALL(minuteHand, isHomed()).WillOnce(Return(true));
    EXPECT_CALL(secondHand, isHomed()).WillOnce(Return(true));
    EXPECT_TRUE(engine.isAllHomed());

    EXPECT_CALL(hourHand, isBusy()).WillOnce(Return(false));
    EXPECT_CALL(minuteHand, isBusy()).WillOnce(Return(true));
    EXPECT_TRUE(engine.isBusy());

    EXPECT_CALL(hourHand, stop()).Times(1);
    EXPECT_CALL(minuteHand, stop()).Times(1);
    EXPECT_CALL(secondHand, stop()).Times(1);
    engine.stopAll();
}
