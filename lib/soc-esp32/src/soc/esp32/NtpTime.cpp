#include "soc/esp32/NtpTime.h"
#include <WiFi.h>
#include "time.h"

static const char* TAG = "NtpTime";

// Define the NTP server to use. "pool.ntp.org" globally or
// for better regional performance, maybe "de.pool.ntp.org" for Germany.
const char *NTP_SERVER = "pool.ntp.org";

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

        bool NtpTime::init(const char *ssid, const char *password)
        {
            // 1. Connect to WiFi
            _logger.i(TAG, "Connecting to WiFi SSID: %s", ssid);
            WiFi.begin(ssid, password);

            // Wait for connection, with a 30-second timeout
            int timeout_seconds = 30;
            while (WiFi.status() != WL_CONNECTED)
            {
                delay(500);
                if (timeout_seconds-- <= 0)
                {
                    ESP_LOGE(TAG, "Failed to connect to WiFi.");
                    return false;
                }
            }
            _logger.i(TAG, "WiFi connected.");

            // 2. Configure time using timezone and NTP server
            // This function sets up the ESP32 to query the NTP server and apply the timezone rule.
            configTzTime(TIME_ZONE, NTP_SERVER);

            // 3. Wait for the time to be synchronized
            _logger.i(TAG, "Synchronizing time with NTP server...");
            struct tm timeinfo;
            // getLocalTime() returns true once time is synced. We'll wait for up to 15 seconds.
            timeout_seconds = 15;
            while (!getLocalTime(&timeinfo))
            {
                delay(500);
                if (timeout_seconds-- <= 0)
                {
                    _logger.e(TAG, "Failed to synchronize time.");
                    _isTimeSynced = false;
                    return false;
                }
            }
            _isTimeSynced = true;
            _logger.i(TAG, "Time synchronized successfully.");

            // Optional: Print current time
            char time_buf[64];
            strftime(time_buf, sizeof(time_buf), "%A, %B %d %Y %H:%M:%S", &timeinfo);
            _logger.i(TAG, "Current time: %s", time_buf);

            return true;
        }

        unsigned long NtpTime::now()
        {
            if (!_isTimeSynced)
            {
                return 0;
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