#pragma once

#include <cstdint>
#include "domain/Time.h"

namespace aviator::domain {

using TimeComponents = Time;

/**
 * @brief Abstract outbound port providing real-time clock and epoch time.
 */
class TimeSource {
public:
    virtual ~TimeSource() = default;

    /**
     * @brief Gets the current epoch time in milliseconds or seconds.
     * @return Current epoch timestamp.
     */
    virtual uint64_t now() = 0;

    /**
     * @brief Translates the current local wall-clock time into broken-down time components.
     * @param components Reference to fill with current hours, minutes, and seconds.
     * @return True if time is valid and synchronized, false otherwise.
     */
    virtual bool asTimeComponents(TimeComponents& components) = 0;
};

} // namespace aviator::domain
