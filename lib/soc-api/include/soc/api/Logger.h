#pragma once

#include <cstdarg> // For va_list, va_start, va_end
#include <cstdio> // For printf on native

namespace soc
{
    namespace api
    {
        enum class LogLevel {
            ERROR = 0,
            WARNING = 1,
            INFO = 2,
            DEBUG = 3
        };

        /**
         * @brief A concrete logging class providing a portable logging interface.
         *
         * This class uses conditional compilation internally to switch between a
         * platform-specific backend (like ESP-IDF's logger) and a standard
         * backend for native host environments (like printf). This keeps all
         * other libraries free of platform-specific preprocessor directives.
         */
        class Logger
        {
        public:
            Logger();

            /**
             * @brief Sets the minimum log level. Messages with a lower priority (higher value) will be ignored.
             * 
             * @param level The new minimum log level.
             */
            void setLogLevel(LogLevel level);

            /**
             * @brief Logs an informational message.
             * @param tag The tag/module issuing the log.
             * @param format The printf-style format string.
             * @param ... The variable arguments for the format string.
             */
            virtual void i(const char *tag, const char *format, ...);
            virtual void w(const char *tag, const char *format, ...);
            virtual void e(const char *tag, const char *format, ...);
            virtual void d(const char *tag, const char *format, ...);

        protected:
            // This is the common entry point for all log messages after formatting.
            virtual void _log_formatted(LogLevel level, const char* tag, const char* formatted_message);

        protected:
            LogLevel m_minLogLevel = LogLevel::DEBUG;
        };
    } // namespace api
} // namespace soc
