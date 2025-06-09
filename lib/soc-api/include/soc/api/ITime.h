    #pragma once

namespace soc
{
    namespace api
    {
        class ITime
        {
        public:
            virtual ~ITime() = default;

            /**
             * @brief Inner structure to hold broken-down time components,
             * representing elapsed time since boot (H:M:S).
             */
            struct TimeComponents
            {
                unsigned long hours; // Hours elapsed since boot (can exceed 23)
                int minutes;         // Minutes part of the elapsed time (0-59)
                int seconds;         // Seconds part of the elapsed time (0-59)
            };

            /**
             * @brief Gets the current time in milliseconds since January 1, 1970, 00:00:00 UTC
             * @return Milliseconds since January 1, 1970, 00:00:00 UTC
             */
            virtual unsigned long now() = 0;

            /**
             * @brief Converts the current uptime (obtained from calling now() internally)
             * into an H:M:S format representing elapsed time.
             * @param time A reference to a TimeComponents struct that will be filled with the
             * calculated elapsed hours, minutes, and seconds since boot.
             */
            virtual bool asTimeComponents(TimeComponents &time) = 0;
        };

    }
}