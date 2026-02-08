#pragma once

#include <cstdint>
#include <iomanip>
#include <sstream>

namespace aviator_clock
{

    struct Time
    {
        uint8_t hours{0};
        uint8_t minutes{0};
        uint8_t seconds{0};

        bool operator==(const Time &other) const
        {
            return hours == other.hours && minutes == other.minutes && seconds == other.seconds;
        }

        std::string toString() const
        {
            std::stringstream ss;
            ss << std::setfill('0') << std::setw(2) << (int)hours << ":"
               << std::setw(2) << (int)minutes << ":"
               << std::setw(2) << (int)seconds;
            return ss.str();
        }
    };

} // namespace aviator_clock
