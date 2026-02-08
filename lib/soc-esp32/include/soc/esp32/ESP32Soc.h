#pragma once

#include <memory>
#include <string>
#include <soc/api/ISoc.h>
#include <soc/api/ITask.h>
#include "ESP32DigitalInput.h"
#include "ESP32Sleeper.h"
#include "ESP32PersistentStorage.h" // Include concrete header
#include "NtpTime.h"
#include <soc/api/Logger.h>

namespace soc
{
    namespace esp32
    {
        /**
         * @brief Concrete implementation of the ISoc interface for the ESP32 platform.
         *
         * This class uses ESP32-specific libraries and the Arduino framework to
         * provide the services defined in the ISoc API.
         */
        class ESP32Soc : public soc::api::ISoc
        {
        public:
            ESP32Soc();
            ~ESP32Soc() override = default;

            /**
             * @brief Initializes the SoC services.
             * Included setting up NTP configuration.
             */
            void init();

            // --- ISoc Interface Implementation ---
            soc::api::Logger &getLogger() override;
            soc::api::ITime &getTimeProvider() override;
            soc::api::ISleeper &getSleeper() override;
            soc::api::IPersistentStorage &getPersistentStorage() override;

            std::unique_ptr<soc::api::IDigitalInput> createDigitalInput(
                int pin, bool pullup) override;
            std::unique_ptr<soc::api::IRotaryEncoder> createRotaryEncoder(
                int pinA, int pinB, int stepsPerNotch = 4) override;

            std::unique_ptr<soc::api::ITask> createTask(
                soc::api::TaskFunction_t taskFunction, const std::string &name,
                unsigned int stackSize = 4096, unsigned int priority = 1) override;

        private:
            // The SoC owns the concrete service implementations.
            soc::api::Logger m_logger;
            std::unique_ptr<NtpTime> m_timeProvider;
            std::unique_ptr<ESP32Sleeper> m_sleeper;
            std::unique_ptr<ESP32PersistentStorage> m_persistentStorage;
        };
    } // namespace esp32
} // namespace soc
