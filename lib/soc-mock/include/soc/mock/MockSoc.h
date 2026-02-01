#pragma once

#include <soc/api/ISoc.h>
#include <soc/mock/MockDigitalInput.h>
#include <soc/mock/MockSleeper.h>
#include <soc/mock/MockTime.h>
#include <soc/api/Logger.h>
#include <memory>
#include <vector>

namespace soc
{
    namespace mock
    {
        /**
         * @brief Mock implementation of the ISoc interface for host-based testing.
         *
         * This class provides mock implementations of all SoC services.
         * It uses std::thread to simulate RTOS tasks.
         */
        class MockSoc : public soc::api::ISoc
        {
        public:
            MockSoc();
            ~MockSoc() override;

            // --- ISoc Interface Implementation ---
            soc::api::Logger &getLogger() override;
            soc::api::ITime &getTimeProvider() override;
            soc::api::ISleeper &getSleeper() override;
            std::unique_ptr<soc::api::IDigitalInput> createDigitalInput(
                int pin, bool pullup) override;
            std::unique_ptr<soc::api::ITask> createTask(
                soc::api::TaskFunction_t taskFunction, const std::string &name,
                unsigned int stackSize = 4096, unsigned int priority = 1) override;

        private:
            // The MockSoc owns the mock service implementations.
            soc::api::Logger m_logger;
            std::unique_ptr<MockTime> m_timeProvider;
            std::unique_ptr<MockSleeper> m_sleeper;
        };
    } // namespace mock
} // namespace soc
