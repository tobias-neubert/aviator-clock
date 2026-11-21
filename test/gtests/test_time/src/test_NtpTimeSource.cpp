#include <gtest/gtest.h>

#include "platform/time/NtpTimeSource.h"

using namespace aviator::platform::time;
using namespace aviator::domain;

namespace {
time_t s_mockTimeEpoch = 0;

time_t mockTimeSupplier(time_t* /* timer */) {
    return s_mockTimeEpoch;
}
} // namespace

class NtpTimeSourceTest : public ::testing::Test {
protected:
    void SetUp() override {
        s_mockTimeEpoch = 0;
    }

    NtpTimeSource timeSource{mockTimeSupplier};
};

TEST_F(NtpTimeSourceTest, InitialState_WithUnsynchronizedTimestamp_ReturnsNotSynchronized) {
    // Epoch 0 corresponds to 1970-01-01 (ESP32 RTC uninitialized state)
    s_mockTimeEpoch = 0;

    EXPECT_FALSE(timeSource.isSynchronized());
}

TEST_F(NtpTimeSourceTest, Now_WhenUnsynchronized_ReturnsZero) {
    s_mockTimeEpoch = 0;

    EXPECT_EQ(timeSource.now(), 0ULL);
}

TEST_F(NtpTimeSourceTest, AsTimeComponents_WhenUnsynchronized_ReturnsFalse) {
    s_mockTimeEpoch = 0;

    TimeComponents components{12, 34, 56};
    const bool success = timeSource.asTimeComponents(components);

    EXPECT_FALSE(success);
    // Values should remain unchanged if translation fails
    EXPECT_EQ(components.hours, 12);
    EXPECT_EQ(components.minutes, 34);
    EXPECT_EQ(components.seconds, 56);
}

TEST_F(NtpTimeSourceTest, Now_WhenSynchronized_ReturnsEpochMilliseconds) {
    // 2026-11-21 12:00:00 UTC
    s_mockTimeEpoch = 1795262400;

    EXPECT_TRUE(timeSource.isSynchronized());
    EXPECT_EQ(timeSource.now(), 1795262400ULL * 1000ULL);
}

TEST_F(NtpTimeSourceTest, AsTimeComponents_WhenSynchronized_TranslatesHoursMinutesSeconds) {
    // Configure UTC timezone for straightforward translation
    NtpConfiguration configUtc{
        .timezone = "UTC0",
        .primaryServer = "pool.ntp.org",
        .secondaryServer = "time.google.com",
        .tertiaryServer = "time.windows.com"
    };
    timeSource.initialize(configUtc);

    // 2026-11-21 12:30:45 UTC = 1795262400 + (30 * 60) + 45 = 1795264245
    s_mockTimeEpoch = 1795264245;

    TimeComponents components{};
    const bool success = timeSource.asTimeComponents(components);

    EXPECT_TRUE(success);
    EXPECT_EQ(components.hours, 12);
    EXPECT_EQ(components.minutes, 30);
    EXPECT_EQ(components.seconds, 45);
}

TEST_F(NtpTimeSourceTest, TimezoneConversion_AppliesPosixTimezoneOffset_WinterCET) {
    // Berlin timezone (CET = UTC+1 in winter)
    NtpConfiguration configBerlin{
        .timezone = "CET-1CEST,M3.5.0,M10.5.0/3",
        .primaryServer = "pool.ntp.org",
        .secondaryServer = "time.google.com",
        .tertiaryServer = "time.windows.com"
    };
    timeSource.initialize(configBerlin);

    // 2026-11-21 12:00:00 UTC -> 13:00:00 CET
    s_mockTimeEpoch = 1795262400;

    TimeComponents components{};
    EXPECT_TRUE(timeSource.asTimeComponents(components));
    EXPECT_EQ(components.hours, 13);
    EXPECT_EQ(components.minutes, 0);
    EXPECT_EQ(components.seconds, 0);
}

TEST_F(NtpTimeSourceTest, TimezoneConversion_AppliesPosixTimezoneOffset_SummerCEST) {
    // Berlin timezone (CEST = UTC+2 in summer)
    NtpConfiguration configBerlin{
        .timezone = "CET-1CEST,M3.5.0,M10.5.0/3",
        .primaryServer = "pool.ntp.org",
        .secondaryServer = "time.google.com",
        .tertiaryServer = "time.windows.com"
    };
    timeSource.initialize(configBerlin);

    // 2026-07-15 12:00:00 UTC -> 14:00:00 CEST
    s_mockTimeEpoch = 1784116800;

    TimeComponents components{};
    EXPECT_TRUE(timeSource.asTimeComponents(components));
    EXPECT_EQ(components.hours, 14);
    EXPECT_EQ(components.minutes, 0);
    EXPECT_EQ(components.seconds, 0);
}

TEST_F(NtpTimeSourceTest, AsTimeComponents_TranslatesMidnightAndEdgeTimesAccurately) {
    NtpConfiguration configUtc{
        .timezone = "UTC0",
        .primaryServer = "pool.ntp.org",
        .secondaryServer = "time.google.com",
        .tertiaryServer = "time.windows.com"
    };
    timeSource.initialize(configUtc);

    // 2026-11-21 00:00:00 UTC = 1795262400 - (12 * 3600) = 1795219200
    s_mockTimeEpoch = 1795219200;

    TimeComponents componentsMidnight{};
    EXPECT_TRUE(timeSource.asTimeComponents(componentsMidnight));
    EXPECT_EQ(componentsMidnight.hours, 0);
    EXPECT_EQ(componentsMidnight.minutes, 0);
    EXPECT_EQ(componentsMidnight.seconds, 0);

    // 2026-11-21 23:59:59 UTC = 1795219200 + 86399 = 1795305599
    s_mockTimeEpoch = 1795305599;

    TimeComponents componentsEndDay{};
    EXPECT_TRUE(timeSource.asTimeComponents(componentsEndDay));
    EXPECT_EQ(componentsEndDay.hours, 23);
    EXPECT_EQ(componentsEndDay.minutes, 59);
    EXPECT_EQ(componentsEndDay.seconds, 59);
}

TEST_F(NtpTimeSourceTest, Initialize_ConfiguresTimezone) {
    NtpConfiguration config{
        .timezone = "EST5EDT,M3.2.0,M11.1.0",
        .primaryServer = "pool.ntp.org",
        .secondaryServer = "time.google.com",
        .tertiaryServer = "time.windows.com"
    };
    EXPECT_TRUE(timeSource.initialize(config));
    EXPECT_STREQ(timeSource.getTimezone(), "EST5EDT,M3.2.0,M11.1.0");
}

TEST_F(NtpTimeSourceTest, SetTimeSupplier_AllowsDynamicallyChangingTimeProvider) {
    EXPECT_FALSE(timeSource.isSynchronized());

    timeSource.setTimeSupplier([](time_t*) -> time_t {
        return 1795262400; // Synchronized year 2026
    });

    EXPECT_TRUE(timeSource.isSynchronized());
    EXPECT_EQ(timeSource.now(), 1795262400ULL * 1000ULL);
}

TEST_F(NtpTimeSourceTest, DefaultTimeSupplier_UsesRealSystemTimeWhenNoSupplierProvided) {
    NtpTimeSource realSystemTimeSource;
    // On the native host running in 2026, real system time is >= 2024, so it should be synchronized
    EXPECT_TRUE(realSystemTimeSource.isSynchronized());
    EXPECT_GT(realSystemTimeSource.now(), 0ULL);

    TimeComponents components{};
    EXPECT_TRUE(realSystemTimeSource.asTimeComponents(components));
    EXPECT_LT(components.hours, 24);
    EXPECT_LT(components.minutes, 60);
    EXPECT_LT(components.seconds, 60);
}
