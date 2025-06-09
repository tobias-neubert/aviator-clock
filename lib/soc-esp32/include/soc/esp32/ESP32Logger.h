#pragma once

#include "soc/api/ILogger.h"
#include <cstdarg>

namespace soc
{
    namespace esp32
    {
        /**
         * @brief Concrete implementation of ILogger using the ESP32's primary Serial port.
         *
         * Writes log messages to the primary Serial port, prefixing them with
         * the corresponding log level. Assumes Serial.begin() has been called elsewhere.
         * Supports printf-style formatting.
         */
        class ESP32Logger : public soc::api::ILogger
        {
        public:
            ESP32Logger(LogLevel minLogLevel = WARN_LEVEL);
            virtual ~ESP32Logger() override = default;

            // Printf-style formatted logging methods
// The __attribute__ provides compile-time format string checking on GCC/Clang.
#ifdef __GNUC__
            __attribute__((format(printf, 2, 3)))
#endif
            void
            trace(const char *format, ...) override;

#ifdef __GNUC__
            __attribute__((format(printf, 2, 3)))
#endif
            void
            debug(const char *format, ...) override;

#ifdef __GNUC__
            __attribute__((format(printf, 2, 3)))
#endif
            void
            info(const char *format, ...) override;

#ifdef __GNUC__
            __attribute__((format(printf, 2, 3)))
#endif
            void
            warn(const char *format, ...) override;

#ifdef __GNUC__
            __attribute__((format(printf, 2, 3)))
#endif
            void
            error(const char *format, ...) override;

        protected:
            /**
             * @brief Helper to perform actual Serial printing.
             * @param level The log level for the message.
             * @param format The printf-style format string.
             * @param args The variable argument list.
             */
            void _log_formatted(LogLevel level, const char *format, va_list args);

        private:
            LogLevel minLogLevel;
        };
    } // namespace esp32
} // namespace soc
