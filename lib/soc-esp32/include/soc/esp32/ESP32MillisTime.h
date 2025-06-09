#pragma once

#include <soc/api/ITime.h>
#include <Arduino.h>

namespace soc
{
    namespace esp32
    {
        class ESP32MillisTime : public soc::api::ITime
        {
        public:
            ESP32MillisTime();
            virtual ~ESP32MillisTime() = default;

            unsigned long now() override;
            bool asTimeComponents(TimeComponents &time) override;
        };

    }
}
