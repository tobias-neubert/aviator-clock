#include <gtest/gtest.h>
#include "domain/Time.h"
#include "domain/Alarm.h"

using namespace aviator::domain;

TEST(TimeTest, DefaultConstructor_InitializesToZero) {
    const Time time;
    EXPECT_EQ(time.hours, 0);
    EXPECT_EQ(time.minutes, 0);
    EXPECT_EQ(time.seconds, 0);
    EXPECT_TRUE(time.isValid());
}

TEST(TimeTest, ParameterizedConstructor_SetsValuesCorrectly) {
    const Time time(14, 35, 42);
    EXPECT_EQ(time.hours, 14);
    EXPECT_EQ(time.minutes, 35);
    EXPECT_EQ(time.seconds, 42);
    EXPECT_TRUE(time.isValid());
}

TEST(TimeTest, EqualityComparison) {
    const Time time1(10, 20, 30);
    const Time time2(10, 20, 30);
    const Time time3(10, 20, 31);

    EXPECT_EQ(time1, time2);
    EXPECT_NE(time1, time3);
}

TEST(TimeTest, ValidityCheck) {
    EXPECT_TRUE(Time(0, 0, 0).isValid());
    EXPECT_TRUE(Time(23, 59, 59).isValid());
    EXPECT_FALSE(Time(24, 0, 0).isValid());
    EXPECT_FALSE(Time(12, 60, 0).isValid());
    EXPECT_FALSE(Time(12, 0, 60).isValid());
}

TEST(AlarmTest, DefaultConstructor_InitializesDefaults) {
    const Alarm alarm;
    EXPECT_EQ(alarm.time, Time(0, 0, 0));
    EXPECT_EQ(alarm.soundTrackId, 1);
    EXPECT_FALSE(alarm.enabled);
}

TEST(AlarmTest, ParameterizedConstructor_SetsValuesCorrectly) {
    const Alarm alarm(Time(7, 30, 0), 4, true);
    EXPECT_EQ(alarm.time, Time(7, 30, 0));
    EXPECT_EQ(alarm.soundTrackId, 4);
    EXPECT_TRUE(alarm.enabled);
}

TEST(AlarmTest, EqualityComparison) {
    const Alarm alarm1(Time(6, 45, 0), 2, true);
    const Alarm alarm2(Time(6, 45, 0), 2, true);
    const Alarm alarm3(Time(6, 45, 0), 3, true);
    const Alarm alarm4(Time(6, 45, 0), 2, false);

    EXPECT_EQ(alarm1, alarm2);
    EXPECT_NE(alarm1, alarm3);
    EXPECT_NE(alarm1, alarm4);
}
