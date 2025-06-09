#pragma once
#include <soc/api/IDigitalOutput.h>

namespace soc
{
    namespace esp32
    {
        class ESP32DigitalOutput : public soc::api::IDigitalOutput
        {
        public:
            explicit ESP32DigitalOutput(int pin);
            void begin() const override;
            void on() override;
            void off() override;

        private:
            int pin;
        };
    }
}
