// soc/esp32/ESP32Logger.cpp
#include "soc/esp32/ESP32Logger.h" // Include the corresponding header
#include <Arduino.h>               // Required for the global Serial object
#include <cstdio>                  // Required for vsnprintf
#include <cstdarg>                 // Required for va_list, va_start, va_copy, va_end
#include <string>                  // Required for std::string methods

namespace soc
{
    namespace esp32
    {

        /**
         * @brief Helper function to get the string prefix for a given log level.
         * Internal linkage only (static).
         * @param level The log level enum value.
         * @return A constant character pointer to the prefix string.
         */
        static const char *getLevelString(soc::api::ILogger::LogLevel level)
        {
            switch (level)
            {
            case soc::api::ILogger::TRACE_LEVEL:
                return "TRACE: ";
            case soc::api::ILogger::DEBUG_LEVEL:
                return "DEBUG: ";
            case soc::api::ILogger::INFO_LEVEL:
                return "INFO: ";
            case soc::api::ILogger::WARN_LEVEL:
                return "WARN: ";
            case soc::api::ILogger::ERROR_LEVEL:
                return "ERROR: ";
            default:
                return "LOG: "; // Fallback for unknown levels
            }
        }

        // --- Constructor ---
        ESP32Logger::ESP32Logger(LogLevel minLevel)
            : minLogLevel(minLevel)
        {
            // Constructor body can be empty or perform other setup if needed
            // Note: Serial.begin() must still be called externally before logging.
        }

        // This function encapsulates the core logic for formatting and printing
        // to avoid repetition in the public variadic methods.
        void ESP32Logger::_log_formatted(LogLevel level, const char *format, va_list args)
        {
            // Essential Check: Ensure Serial port is initialized and available.
            if (!Serial)
            {
                return; // Cannot log if Serial isn't ready
            }

            // --- Runtime Log Level Filtering ---
            if (level < minLogLevel)
            {
                return; // Ignore messages below the configured level
            }
            // --- End Filtering ---

            // --- Safely format the message using vsnprintf ---
            char buffer[256]; // Define buffer size
            char *buf_ptr = buffer;

            va_list args_copy;
            va_copy(args_copy, args);
            int required_len = vsnprintf(buf_ptr, sizeof(buffer), format, args_copy);
            va_end(args_copy);

            // --- Handle potential formatting errors or buffer truncation ---
            if (required_len < 0)
            {
                Serial.print(getLevelString(ERROR_LEVEL));
                Serial.println("Log formatting error!");
                return;
            }
            // Optional: Check for truncation.
            // if (required_len >= sizeof(buffer)) { ... }

            // --- Print the final formatted message to Serial ---
            Serial.print(getLevelString(level)); // Print level prefix
            Serial.println(buf_ptr);             // Print the formatted message

            // Optional: Flush the serial buffer for important messages
            // if (level >= WARN_LEVEL) { Serial.flush(); }
        }

        // --- Variadic Methods Implementation ---
        // These methods handle the va_list setup and call the internal helper.
#ifdef __GNUC__
        __attribute__((format(printf, 2, 3)))
#endif
        void
        ESP32Logger::trace(const char *format, ...)
        {
            if (TRACE_LEVEL >= minLogLevel)
            {
                va_list args;
                va_start(args, format);
                _log_formatted(TRACE_LEVEL, format, args);
                va_end(args);
            }
        }

#ifdef __GNUC__
        __attribute__((format(printf, 2, 3)))
#endif
        void
        ESP32Logger::debug(const char *format, ...)
        {
            if (DEBUG_LEVEL >= minLogLevel)
            {
                va_list args;
                va_start(args, format);
                _log_formatted(DEBUG_LEVEL, format, args);
                va_end(args);
            }
        }

#ifdef __GNUC__
        __attribute__((format(printf, 2, 3)))
#endif
        void
        ESP32Logger::info(const char *format, ...)
        {
            if (INFO_LEVEL >= minLogLevel)
            {
                va_list args;
                va_start(args, format);
                _log_formatted(INFO_LEVEL, format, args);
                va_end(args);
            }
        }

#ifdef __GNUC__
        __attribute__((format(printf, 2, 3)))
#endif
        void
        ESP32Logger::warn(const char *format, ...)
        {
            if (WARN_LEVEL >= minLogLevel)
            {
                va_list args;
                va_start(args, format);
                _log_formatted(WARN_LEVEL, format, args);
                va_end(args);
            }
        }

#ifdef __GNUC__
        __attribute__((format(printf, 2, 3)))
#endif
        void
        ESP32Logger::error(const char *format, ...)
        {
            if (ERROR_LEVEL >= minLogLevel)
            {
                va_list args;
                va_start(args, format);
                _log_formatted(ERROR_LEVEL, format, args);
                va_end(args);
            }
        }
    } // namespace esp32
} // namespace soc
