#include <soc/esp32/ESP32DigitalOutput.h>
#include <Arduino.h>

namespace soc
{
    namespace esp32
    {
        ESP32DigitalOutput::ESP32DigitalOutput(int pin) : pin(pin)
        {
        }

        void ESP32DigitalOutput::begin() const
        {
            pinMode(pin, OUTPUT);
        }

        void ESP32DigitalOutput::on()
        {
            digitalWrite(pin, HIGH);
        }

        void ESP32DigitalOutput::off()
        {
            digitalWrite(pin, LOW);
        }
    }
}
