#include <gtest/gtest.h>
#include "domain/DialGeometry.h"

using namespace aviator::domain;

class DialGeometryTest : public ::testing::Test {
protected:
    DialGeometry geometry{330.0};
};

TEST_F(DialGeometryTest, DefaultActiveArcIs330Degrees) {
    EXPECT_DOUBLE_EQ(geometry.getActiveArcDegrees(), 330.0);
    EXPECT_DOUBLE_EQ(geometry.getDegreesPerHour(), 27.5);
    EXPECT_DOUBLE_EQ(geometry.getDegreesPerMinute(), 5.5);
    EXPECT_DOUBLE_EQ(geometry.getDegreesPerSecond(), 5.5);
}

TEST_F(DialGeometryTest, HourToDegrees_Converts12HourScaleCorrectly) {
    EXPECT_DOUBLE_EQ(geometry.hourToDegrees(0), 0.0);
    EXPECT_DOUBLE_EQ(geometry.hourToDegrees(1), 27.5);
    EXPECT_DOUBLE_EQ(geometry.hourToDegrees(6), 165.0);
    EXPECT_DOUBLE_EQ(geometry.hourToDegrees(11), 302.5);
    EXPECT_DOUBLE_EQ(geometry.hourToDegrees(12), 0.0);
    EXPECT_DOUBLE_EQ(geometry.hourToDegrees(18), 165.0);
    EXPECT_DOUBLE_EQ(geometry.hourToDegrees(23), 302.5);
}

TEST_F(DialGeometryTest, MinuteToDegrees_Converts60UnitScaleCorrectly) {
    EXPECT_DOUBLE_EQ(geometry.minuteToDegrees(0), 0.0);
    EXPECT_DOUBLE_EQ(geometry.minuteToDegrees(1), 5.5);
    EXPECT_DOUBLE_EQ(geometry.minuteToDegrees(15), 82.5);
    EXPECT_DOUBLE_EQ(geometry.minuteToDegrees(30), 165.0);
    EXPECT_DOUBLE_EQ(geometry.minuteToDegrees(45), 247.5);
    EXPECT_DOUBLE_EQ(geometry.minuteToDegrees(59), 324.5);
}

TEST_F(DialGeometryTest, SecondToDegrees_Converts60UnitScaleCorrectly) {
    EXPECT_DOUBLE_EQ(geometry.secondToDegrees(0), 0.0);
    EXPECT_DOUBLE_EQ(geometry.secondToDegrees(1), 5.5);
    EXPECT_DOUBLE_EQ(geometry.secondToDegrees(30), 165.0);
    EXPECT_DOUBLE_EQ(geometry.secondToDegrees(59), 324.5);
}

TEST_F(DialGeometryTest, WrapAroundDetection) {
    EXPECT_TRUE(geometry.isHourWrapAround(11, 0));
    EXPECT_TRUE(geometry.isHourWrapAround(23, 0));
    EXPECT_FALSE(geometry.isHourWrapAround(10, 11));
    EXPECT_FALSE(geometry.isHourWrapAround(0, 1));

    EXPECT_TRUE(geometry.isMinuteWrapAround(59, 0));
    EXPECT_FALSE(geometry.isMinuteWrapAround(58, 59));
    EXPECT_FALSE(geometry.isMinuteWrapAround(0, 1));

    EXPECT_TRUE(geometry.isSecondWrapAround(59, 0));
    EXPECT_FALSE(geometry.isSecondWrapAround(58, 59));
    EXPECT_FALSE(geometry.isSecondWrapAround(0, 1));
}

TEST_F(DialGeometryTest, CalculateTargetAngle_WithWrapAroundReturnsFullCircle) {
    // Normal progress
    EXPECT_DOUBLE_EQ(geometry.calculateMinuteTargetAngle(31, 30), 31 * 5.5);
    EXPECT_DOUBLE_EQ(geometry.calculateSecondTargetAngle(15, 14), 15 * 5.5);
    EXPECT_DOUBLE_EQ(geometry.calculateHourTargetAngle(2, 1), 2 * 27.5);

    // Forward clockwise wrap-around jumping (59->0 and 11->0)
    EXPECT_DOUBLE_EQ(geometry.calculateMinuteTargetAngle(0, 59), DialGeometry::FULL_CIRCLE_DEGREES);
    EXPECT_DOUBLE_EQ(geometry.calculateSecondTargetAngle(0, 59), DialGeometry::FULL_CIRCLE_DEGREES);
    EXPECT_DOUBLE_EQ(geometry.calculateHourTargetAngle(0, 11), DialGeometry::FULL_CIRCLE_DEGREES);
    EXPECT_DOUBLE_EQ(geometry.calculateHourTargetAngle(0, 23), DialGeometry::FULL_CIRCLE_DEGREES);

    // Direct jump to 0 not following 59 or 11
    EXPECT_DOUBLE_EQ(geometry.calculateMinuteTargetAngle(0, 30), 0.0);
    EXPECT_DOUBLE_EQ(geometry.calculateHourTargetAngle(0, 5), 0.0);
}

TEST_F(DialGeometryTest, AlarmPeriodIndicator_ReturnsCorrectAngles) {
    EXPECT_DOUBLE_EQ(geometry.alarmPeriodIndicatorToDegrees(false), 0.0);
    EXPECT_DOUBLE_EQ(geometry.alarmPeriodIndicatorToDegrees(true), 165.0);
}
