#pragma once
#include <soc/api/ISleeper.h>

namespace soc
{
    namespace esp32
    {
        class ESP32Sleeper : public soc::api::ISleeper
        {
        public:
            explicit ESP32Sleeper();
            void sleep(uint32_t millis) override;

        private:
            int pin;
        };
    }
}
