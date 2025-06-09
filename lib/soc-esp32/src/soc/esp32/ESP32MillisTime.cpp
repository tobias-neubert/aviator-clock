#include <soc/api/ITime.h>
#include <soc/esp32/ESP32MillisTime.h>

namespace soc
{
    namespace esp32
    {
        ESP32MillisTime::ESP32MillisTime()
        {
        }

        unsigned long ESP32MillisTime::now()
        {
            return millis();
        }

        bool ESP32MillisTime::asTimeComponents(soc::api::ITime::TimeComponents &time)
        {
            unsigned long currentMillis = millis();

            unsigned long totalSeconds = currentMillis / 1000;

            time.seconds = totalSeconds % 60;

            unsigned long totalMinutes = totalSeconds / 60;
            time.minutes = totalMinutes % 60;

            unsigned long totalHours = totalMinutes / 60;
            time.hours = totalHours;

            return true;
        }

    }
}