// soc/api/ISoc.h
#pragma once

#include <memory> // For std::shared_ptr
#include <soc/api/ISocComponent.h>

namespace soc
{
    namespace api
    {
        /**
         * @brief Interface representing the main application.
         *
         * Manages a collection of ISocComponents and orchestrates the main application
         * loop phases (Input, State Advancement, Rendering).
         */
        class ISoc
        {
        public:
            /**
             * @brief Virtual destructor is essential for interfaces.
             */
            virtual ~ISoc() = default;

            /**
             * @brief Processes world-level inputs (e.g., network server events,
             * system-wide button presses). This phase typically happens once per loop.
             * It might trigger actions or state changes within specific components later
             * during the advanceState phase.
             */
            virtual void processInput() = 0;

            /**
             * @brief Advances the state of all managed components based on time.
             * Typically iterates through all registered ISocComponents and calls their
             * update() method.
             * @param currentTimeMs The current application time in milliseconds.
             */
            virtual void advanceState(unsigned long currentTimeMs) = 0;

            /**
             * @brief Renders the current state of all managed components.
             * Typically iterates through all registered ISocComponents and calls their
             * render() method. This is where components produce their output (e.g.,
             * update LEDs, displays, send data).
             */
            virtual void render() = 0;

            /**
             * @brief Adds a component to be managed by the ISoc.
             *
             * Once added, the component's setup() method will likely be called,
             * and its update() and render() methods will be called during the
             * corresponding phases of the main loop. Its teardown() method
             * may be called upon removal or world destruction.
             *
             * @param component A std::shared_ptr to the component implementing ISocComponent.
             * Using shared_ptr helps manage the lifetime of the component.
             */
            virtual void addComponent(std::shared_ptr<ISocComponent> component) = 0;
        };

    } // namespace api
} // namespace soc