#include <gtest/gtest.h>
#include "aviator-clock/TimeEditor.h"

using namespace testing;
using namespace aviator_clock;

class TimeEditorTest : public Test {
protected:
    TimeEditor editor;
};

TEST_F(TimeEditorTest, InitialTimeIsZero) {
    Time t = editor.getTime();
    EXPECT_EQ(0, t.hours);
    EXPECT_EQ(0, t.minutes);
    EXPECT_EQ(0, t.seconds);
}

TEST_F(TimeEditorTest, AdjustMinutesPositively) {
    editor.adjustMinutes(10);
    EXPECT_EQ(10, editor.getTime().minutes);
}

TEST_F(TimeEditorTest, AdjustMinutesWrapAroundPositive) {
    editor.setTime({0, 55, 0});
    editor.adjustMinutes(10);
    EXPECT_EQ(5, editor.getTime().minutes);
}

TEST_F(TimeEditorTest, AdjustMinutesWrapAroundNegative) {
    editor.setTime({0, 5, 0});
    editor.adjustMinutes(-10);
    EXPECT_EQ(55, editor.getTime().minutes);
}

TEST_F(TimeEditorTest, AdjustMinutesLargeDelta) {
    editor.adjustMinutes(130); // +2 hours and 10 mins
    EXPECT_EQ(10, editor.getTime().minutes);
    // Hours should NOT change when adjusting minutes (usually encoders are separate)
    EXPECT_EQ(0, editor.getTime().hours);
}

TEST_F(TimeEditorTest, AdjustHoursPositively) {
    editor.adjustHours(5);
    EXPECT_EQ(5, editor.getTime().hours);
}

TEST_F(TimeEditorTest, AdjustHoursWrapAroundPositive) {
    editor.setTime({20, 0, 0});
    editor.adjustHours(5);
    EXPECT_EQ(1, editor.getTime().hours); // 20 + 5 = 25 -> 1
}

TEST_F(TimeEditorTest, AdjustHoursWrapAroundNegative) {
    editor.setTime({2, 0, 0});
    editor.adjustHours(-5);
    EXPECT_EQ(21, editor.getTime().hours); // 2 - 5 = -3 -> 21
}

TEST_F(TimeEditorTest, IsAM_ReturnsTrueForMorning) {
    editor.setTime({8, 0, 0});
    EXPECT_TRUE(editor.isAM());
}

TEST_F(TimeEditorTest, IsAM_ReturnsFalseForEvening) {
    editor.setTime({20, 0, 0});
    EXPECT_FALSE(editor.isAM());
}

TEST_F(TimeEditorTest, GetSecondHandTarget_Returns0_ForAM) {
    editor.setTime({8, 0, 0});
    EXPECT_EQ(0, editor.getSecondHandTarget());
}

TEST_F(TimeEditorTest, GetSecondHandTarget_Returns30_ForPM) {
    editor.setTime({20, 0, 0});
    EXPECT_EQ(30, editor.getSecondHandTarget());
}

TEST_F(TimeEditorTest, GetDisplay12h_Returns12For0Hours) {
    editor.setTime({0, 0, 0});
    EXPECT_EQ(12, editor.getDisplay12h());
}

TEST_F(TimeEditorTest, GetDisplay12h_Returns12For12Hours) {
    editor.setTime({12, 0, 0});
    EXPECT_EQ(12, editor.getDisplay12h());
}

TEST_F(TimeEditorTest, GetDisplay12h_Returns8For20Hours) {
    editor.setTime({20, 0, 0});
    EXPECT_EQ(8, editor.getDisplay12h());
}
