#pragma once

#include <cstdint>

namespace soc
{
    namespace api
    {

        /**
         * @brief Interface for a rotary encoder.
         *
         * Provides abstraction for reading input from a rotary encoder.
         * It focuses on retrieving the relative movement (delta) since the last check.
         */
        class IRotaryEncoder
        {
        public:
            virtual ~IRotaryEncoder() = default;

            /**
             * @brief Updates the encoder state.
             *
             * Need to be called frequently if the implementation is polling-based.
             * Can be a no-op if implementation is interrupt-based.
             */
            virtual void update() = 0;

            /**
             * @brief Gets the change in position since the last call to getDelta().
             *
             * Calling this method typically resets the internal delta counter,
             * so that subsequent calls return changes relative to this moment.
             *
             * @return The number of steps (positive for CW, negative for CCW) moved.
             */
            virtual int32_t getDelta() = 0;

            /**
             * @brief Resets the absolute position counter (if maintained) and delta.
             */
            virtual void reset() = 0;
        };

    } // namespace api
} // namespace soc
