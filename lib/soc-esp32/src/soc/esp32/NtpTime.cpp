#include "soc/esp32/NtpTime.h"
#include <WiFi.h>
#include "time.h"

static const char* TAG = "NtpTime";

// Define the NTP server to use. "pool.ntp.org" globally or
// for better regional performance, maybe "de.pool.ntp.org" for Germany.
const char *NTP_SERVER_1 = "pool.ntp.org";
const char *NTP_SERVER_2 = "time.google.com";
const char *NTP_SERVER_3 = "time.windows.com";

// Timezone configuration for Germany (Europe/Berlin).
// CET-1CEST,M3.5.0,M10.5.0/3
// CET-1: Central European Time is 1 hour ahead of UTC. (The sign is inverted in POSIX timezone strings)
// CEST: Daylight Saving Time name is CEST.
// M3.5.0: DST starts on the last Sunday of March.
// M10.5.0/3: DST ends on the last Sunday of October at 3:00 AM.
const char *TIME_ZONE = "CET-1CEST,M3.5.0,M10.5.0/3";

namespace soc
{
    namespace esp32
    {
        NtpTime::NtpTime(soc::api::Logger& logger) : _logger(logger), _isTimeSynced(false)
        {
        }

        bool NtpTime::init()
        {
            // Configure time using timezone and NTP server
            // This function sets up the ESP32 to query the NTP server and apply the timezone rule.
            // It runs in the background and will sync whenever WiFi is available.
            configTzTime(TIME_ZONE, NTP_SERVER_1, NTP_SERVER_2, NTP_SERVER_3);
            
            _logger.i(TAG, "NTP Time configured. Waiting for WiFi to sync...");

            return true;
        }

        unsigned long NtpTime::now()
        {
            if (!_isTimeSynced)
            {
                struct tm timeinfo;
                // Check if we are synced now
                if (getLocalTime(&timeinfo, 0)) {
                    _isTimeSynced = true;
                    _logger.i(TAG, "Time synced via NTP.");
                } else {
                    return 0;
                }
            }

            struct timeval timeValue;
            // gettimeofday() gives us seconds and microseconds since epoch
            gettimeofday(&timeValue, nullptr);

            // Convert seconds and microseconds to milliseconds
            unsigned long long millisecondsSinceEpoch =
                (unsigned long long)(timeValue.tv_sec) * 1000 +
                (unsigned long long)(timeValue.tv_usec) / 1000;

            return (unsigned long)millisecondsSinceEpoch;
        }

        bool NtpTime::asTimeComponents(TimeComponents &time)
        {
            if (!_isTimeSynced)
            {
                return false;
            }

            struct tm timeinfo;
            if (!getLocalTime(&timeinfo))
            {
                return false;
            }

            time.hours = timeinfo.tm_hour;
            time.minutes = timeinfo.tm_min;
            time.seconds = timeinfo.tm_sec;
            return true;
        }

        unsigned long NtpTime::millis() const
        {
            return ::millis();
        }
    } // namespace esp32
} // namespace soc