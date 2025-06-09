#pragma once
#include <cstdint>

namespace soc
{
    namespace api
    {
        /// Abstract interface for a millisecond delay provider.
        class ISleeper
        {
        public:
            /// Pause execution for the given number of milliseconds.
            virtual void sleep(uint32_t millis) = 0;

            /// Virtual destructor to allow safe polymorphic cleanup.
            virtual ~ISleeper() = default;
        };
    }
}