#pragma once

#include "aviator-clock/Time.h"
#include <cmath>

namespace aviator_clock
{

    class TimeEditor
    {
    public:
        explicit TimeEditor(const Time &initialTime = {0, 0, 0}) : _currentTime(initialTime) {}

        void setTime(const Time &time)
        {
            _currentTime = time;
        }

        void adjustHours(int delta)
        {
            int newHours = _currentTime.hours + delta;

            // Handle wrap around 0-23
            while (newHours < 0)
                newHours += 24;
            while (newHours >= 24)
                newHours -= 24;

            _currentTime.hours = static_cast<uint8_t>(newHours);
        }

        void adjustMinutes(int delta)
        {
            int newMinutes = _currentTime.minutes + delta;

            // Handle wrap around 0-59
            while (newMinutes < 0)
                newMinutes += 60;
            while (newMinutes >= 60)
                newMinutes -= 60;

            _currentTime.minutes = static_cast<uint8_t>(newMinutes);
        }

        Time getTime() const
        {
            return _currentTime;
        }

        bool isAM() const
        {
            return _currentTime.hours < 12;
        }

        uint8_t getDisplay12h() const
        {
            uint8_t h = _currentTime.hours % 12;
            return (h == 0) ? 12 : h; // Usually clocks show 12 instead of 0
        }

        /**
         * @brief Returns the position for the second hand based on AM/PM.
         * @return 0 for AM, 30 for PM.
         */
        uint8_t getSecondHandTarget() const
        {
            return isAM() ? 0 : 30;
        }

    private:
        Time _currentTime;
    };

} // namespace aviator_clock
