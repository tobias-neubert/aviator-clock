#pragma once

#include <memory>
#include <string>
#include "IDigitalInput.h"
#include "ISleeper.h"
#include "ITask.h"
#include "ITime.h"
#include "Logger.h"
#include "IPersistentStorage.h"

namespace soc
{
    namespace api
    {
        /**
         * @brief Interface for the System-on-a-Chip (SoC) abstraction layer.
         *
         * This class acts as a facade and factory for all hardware-specific services,
         * such as logging, time providers, and I/O. It decouples the application
         * logic from the concrete hardware implementation.
         */
        class ISoc
        {
        public:
            virtual ~ISoc() = default;

            /**
             * @brief Gets a reference to the system-wide logger.
             */
            virtual Logger &getLogger() = 0;

            /**
             * @brief Gets a reference to the system-wide time provider.
             */
            virtual ITime &getTimeProvider() = 0;

            /**
             * @brief Gets a reference to the system-wide sleeper.
             */
            virtual ISleeper &getSleeper() = 0;
            
            /**
             * @brief Gets a reference to the persistent storage.
             */
            virtual IPersistentStorage &getPersistentStorage() = 0;

            /**
             * @brief Factory method to create a digital input pin.
             *
             * @param pin The GPIO pin number.
             * @param pullup Whether to enable the internal pull-up resistor.
             * @return A unique_ptr to the created IDigitalInput instance.
             */
            virtual std::unique_ptr<IDigitalInput> createDigitalInput(int pin,
                                                                      bool pullup) = 0;

            /**
             * @brief Factory method to create and start a new RTOS task.
             *
             * The created task will begin execution immediately. The lifecycle of the
             * underlying OS task is tied to the returned unique_ptr. When the pointer
             * is destroyed, the task should be gracefully terminated and joined.
             *
             * @param taskFunction The function or lambda to be executed by the task.
             * @param name A descriptive name for the task (used for debugging).
             * @param stackSize The stack size in bytes to be allocated for the task.
             * @param priority The priority at which the task should run (higher numbers
             * mean higher priority).
             * @return A unique_ptr to an ITask object representing the running task.
             */
            virtual std::unique_ptr<ITask> createTask(TaskFunction_t taskFunction,
                                                      const std::string &name,
                                                      unsigned int stackSize = 4096,
                                                      unsigned int priority = 1) = 0;
        };
    } // namespace api
} // namespace soc
