#include <soc/esp32/ESP32Sleeper.h>
#include <Arduino.h>

namespace soc
{
    namespace esp32
    {
        ESP32Sleeper::ESP32Sleeper()
        {
        }

        void ESP32Sleeper::sleep(uint32_t millis)
        {
            ::delay(millis);
        }
    }
}
