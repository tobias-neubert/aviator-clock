#pragma once

#include "soc/api/ITime.h"

namespace soc
{
    namespace esp32
    {
        class NtpTime : public api::ITime
        {
        public:
            NtpTime();
            virtual ~NtpTime() = default;

            /**
             * @brief Initializes the component, connecting to WiFi and setting up the NTP client.
             * @param ssid The SSID of the WiFi network.
             * @param password The password for the WiFi network.
             * @return True if initialization (WiFi + NTP sync) was successful, false otherwise.
             */
            bool init(const char *ssid, const char *password);

            /**
             * @brief Gets the current time in milliseconds since January 1, 1970, 00:00:00 UTC
             * @return Milliseconds since January 1, 1970, 00:00:00 UTC. Returns 0 if time is not yet synced.
             */
            unsigned long now() override;

            /**
             * @brief Gets the current time of day in H:M:S format.
             * @note This implementation provides the current time of day (e.g., 14:30:15) rather than
             * time since boot, as this is more useful for a real-time clock application.
             * @param time A reference to a TimeComponents struct that will be filled with the
             * current hour (0-23), minute (0-59), and second (0-59).
             * @return True if time is available and the struct was filled, false if time is not yet synced.
             */
            bool asTimeComponents(TimeComponents &time) override;

        private:        
            bool _isTimeSynced;
        };
    } // namespace esp32
} // namespace soc