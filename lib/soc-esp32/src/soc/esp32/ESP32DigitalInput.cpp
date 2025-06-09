#include <soc/esp32/ESP32DigitalInput.h>

namespace soc
{
    namespace esp32
    {
        ESP32DigitalInput::ESP32DigitalInput(uint8_t pin, bool activeLow)
            : _pin(pin), _activeLow(activeLow)
        {
        }

        void ESP32DigitalInput::begin() const
        {
            // If activeLow is true, we assume the switch connects the pin to GND when active,
            // so we use INPUT_PULLUP to keep the pin HIGH when the switch is open.
            // If activeLow is false, we assume the switch connects the pin to VCC when active,
            // so we use INPUT_PULLDOWN to keep the pin LOW when the switch is open.
            pinMode(_pin, _activeLow ? INPUT_PULLUP : INPUT_PULLDOWN);
        }

        bool ESP32DigitalInput::isActive() const
        {
            int pinState = digitalRead(_pin);
            return _activeLow ? (pinState == LOW) : (pinState == HIGH);
        }
    }
}