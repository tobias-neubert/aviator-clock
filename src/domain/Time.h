#pragma once

#include <cstdint>

namespace aviator::domain {

/**
 * @brief Plain data struct representing standard time components.
 */
struct Time {
    uint8_t hours{0};
    uint8_t minutes{0};
    uint8_t seconds{0};

    constexpr Time() = default;

    constexpr Time(uint8_t hours, uint8_t minutes, uint8_t seconds)
        : hours(hours), minutes(minutes), seconds(seconds) {}

    bool operator==(const Time& other) const {
        return hours == other.hours && minutes == other.minutes && seconds == other.seconds;
    }

    bool operator!=(const Time& other) const {
        return !(*this == other);
    }

    bool isValid() const {
        return hours < 24 && minutes < 60 && seconds < 60;
    }
};

} // namespace aviator::domain
