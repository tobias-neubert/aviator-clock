#pragma once

#include <cstdint>
#include "domain/Time.h"

namespace aviator::domain {

/**
 * @brief Plain data struct representing an alarm configuration.
 */
struct Alarm {
    Time time{};
    uint16_t soundTrackId{1};
    bool enabled{false};

    constexpr Alarm() = default;

    constexpr Alarm(const Time& time, uint16_t soundTrackId, bool enabled)
        : time(time), soundTrackId(soundTrackId), enabled(enabled) {}

    bool operator==(const Alarm& other) const {
        return time == other.time &&
               soundTrackId == other.soundTrackId &&
               enabled == other.enabled;
    }

    bool operator!=(const Alarm& other) const {
        return !(*this == other);
    }
};

} // namespace aviator::domain
