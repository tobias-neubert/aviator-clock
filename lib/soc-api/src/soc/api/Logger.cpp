#include "soc/api/Logger.h"
#include <cstdarg>
#include <cstdio> // For printf on native

#if defined(ESP32) || defined(ESP_PLATFORM) || defined(ARDUINO_ARCH_ESP32)
    #ifndef ESP32
        #define ESP32
    #endif
    // Include Arduino.h to access Serial
    #include <Arduino.h>
#endif

namespace soc
{
    namespace api
    {
        Logger::Logger() : m_minLogLevel(LogLevel::DEBUG) 
        {
        }

        void Logger::setLogLevel(LogLevel level)
        {
            m_minLogLevel = level;
        }

        void Logger::i(const char *tag, const char *format, ...)
        {
            if (LogLevel::INFO > m_minLogLevel) return;

            va_list args;
            va_start(args, format);
            char buffer[256];
            vsnprintf(buffer, sizeof(buffer), format, args);
            va_end(args);
            _log_formatted(LogLevel::INFO, tag, buffer);
        }

        void Logger::w(const char *tag, const char *format, ...)
        {
            if (LogLevel::WARNING > m_minLogLevel) return;

            va_list args;
            va_start(args, format);
            char buffer[256];
            vsnprintf(buffer, sizeof(buffer), format, args);
            va_end(args);
            _log_formatted(LogLevel::WARNING, tag, buffer);
        }

        void Logger::e(const char *tag, const char *format, ...)
        {
            if (LogLevel::ERROR > m_minLogLevel) return;

            va_list args;
            va_start(args, format);
            char buffer[256];
            vsnprintf(buffer, sizeof(buffer), format, args);
            va_end(args);
            _log_formatted(LogLevel::ERROR, tag, buffer);
        }

        void Logger::d(const char *tag, const char *format, ...)
        {
            if (LogLevel::DEBUG > m_minLogLevel) return;

            va_list args;
            va_start(args, format);
            char buffer[256];
            vsnprintf(buffer, sizeof(buffer), format, args);
            va_end(args);
            _log_formatted(LogLevel::DEBUG, tag, buffer);
        }

        void Logger::_log_formatted(LogLevel level, const char* tag, const char* formatted_message) {
#ifdef ESP32
            // We use Serial.printf ensuring output goes to the initialized Serial port.
            // Check if Serial is initialized? Hard to know, but usually safe to call.
            
            const char* level_char = "V";
            switch(level) {
                case LogLevel::INFO:    level_char = "I"; break;
                case LogLevel::WARNING: level_char = "W"; break;
                case LogLevel::ERROR:   level_char = "E"; break;
                case LogLevel::DEBUG:   level_char = "D"; break;
                default:                level_char = "V"; break;
            }
            
            // Format: [Level][Tag]: Message
            // Using millis() for a simple timestamp
            Serial.printf("[%lu][%s][%s]: %s\n", millis(), level_char, tag, formatted_message);

#else
            // Native logging (e.g., to console)
            const char* level_str;
            switch(level) {
                case LogLevel::INFO: level_str = "INFO"; break;
                case LogLevel::WARNING: level_str = "WARN"; break;
                case LogLevel::ERROR: level_str = "ERROR"; break;
                case LogLevel::DEBUG: level_str = "DEBUG"; break;
                default: level_str = "UNKNOWN"; break;
            }
            printf("[%s] %s: %s\n", tag, level_str, formatted_message);
#endif
        }
    } // namespace api
} // namespace soc
