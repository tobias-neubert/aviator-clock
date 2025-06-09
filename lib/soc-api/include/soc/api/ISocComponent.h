// soc/api/ISocComponent.h
#pragma once

namespace soc
{
    namespace api
    {
        /**
         * @brief Interface for components managed by an ISoc implementation.
         *
         * Components represent distinct parts of the application logic or hardware interaction
         * that need to be processed within the main loop (update state, render output).
         */
        class ISocComponent
        {
        public:
            /**
             * @brief Virtual destructor is essential for interfaces.
             */
            virtual ~ISocComponent() = default;

            /**
             * @brief Called once when the component is added to the ISoc world.
             * Override this to perform any necessary component-specific setup or
             * resource initialization.
             */
            virtual void setup() {}

            /**
             * @brief Called repeatedly during the ISoc::advanceState phase.
             * Implement this method to update the component's internal state
             * based on the provided time.
             * @param currentTimeMs Current application time in milliseconds, typically
             * derived from a monotonic clock like millis().
             */
            virtual void advanceState(unsigned long currentTimeMs) = 0;

            /**
             * @brief Called repeatedly during the ISoc::render phase.
             * Implement this method to produce any output (e.g., write to hardware,
             * update display) based on the component's current state.
             */
            virtual void render() = 0;

            /**
             * @brief Called once when the component is about to be removed from the
             * ISoc world or when the world itself is being destroyed.
             * Override this to perform any necessary cleanup or resource deallocation.
             */
            virtual void teardown() {}
        };

    } // namespace api
} // namespace soc