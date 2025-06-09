#pragma once

namespace soc
{
    namespace api
    {
        class IDigitalInput
        {
        public:
            virtual ~IDigitalInput() = default;

            /**
             * @brief Checks if the digital input is currently active.
             * "Active" could mean HIGH or LOW depending on the sensor and wiring.
             * The implementation of this interface will handle that logic.
             * @return True if the input is considered active (e.g., switch pressed), false otherwise.
             */
            virtual bool isActive() const = 0;

            virtual void begin() const = 0;
        };
    }
}