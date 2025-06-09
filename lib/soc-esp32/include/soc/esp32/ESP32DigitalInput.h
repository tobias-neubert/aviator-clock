// include/platform/ESP32DigitalInput.h
#pragma once

#include <soc/api/IDigitalInput.h>
#include <Arduino.h>

namespace soc
{
    namespace esp32
    {
        class ESP32DigitalInput : public soc::api::IDigitalInput
        {
        public:
            /**
             * @brief Constructor for ESP32DigitalInput.
             * @param pin The GPIO pin number to use for the digital input.
             * @param activeLow True if the input is considered "active" when the pin state is LOW
             * (e.g., for a switch pulling to GND with INPUT_PULLUP).
             * False if active HIGH (e.g., for a switch pulling to VCC with INPUT_PULLDOWN).
             */
            ESP32DigitalInput(uint8_t pin, bool activeLow = true);

            /**
             * @brief Initializes the GPIO pin mode (e.g., INPUT_PULLUP or INPUT_PULLDOWN).
             * This should be called once, typically in a setup routine.
             */
            void begin() const override;

            /**
             * @brief Checks if the digital input is currently active.
             * @return True if the input is active based on its configured logic, false otherwise.
             */
            bool isActive() const override;

        private:
            uint8_t _pin;
            bool _activeLow;
        };
    }
}