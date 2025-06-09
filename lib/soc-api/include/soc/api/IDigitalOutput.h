#pragma once

namespace soc
{
    namespace api
    {
        class IDigitalOutput
        {
        public:
            virtual void on() = 0;
            virtual void off() = 0;

            virtual ~IDigitalOutput() = default;

            virtual void begin() const = 0;
        };
    }
}