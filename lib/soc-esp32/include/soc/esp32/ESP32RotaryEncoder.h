#pragma once

#include <soc/api/IRotaryEncoder.h>
#include <ESP32Encoder.h>

namespace soc
{
    namespace esp32
    {
        class ESP32RotaryEncoder : public soc::api::IRotaryEncoder
        {
        public:
            ESP32RotaryEncoder(int pinA, int pinB, int stepsPerNotch);
            ~ESP32RotaryEncoder();

            void update() override; // Polling if needed, but ESP32Encoder is ISR based
            int32_t getDelta() override;
            void reset() override;

        private:
            ESP32Encoder _encoder;
            int64_t _lastPosition{0};
            int _stepsPerNotch{4};
        };

    } // namespace esp32
} // namespace soc
