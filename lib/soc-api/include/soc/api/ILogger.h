// soc/api/ILogger.h
#pragma once

#include <string>
#include <cstdarg> // For va_list usage in derived classes

namespace soc
{
    namespace api
    {

        /**
         * @brief Interface for logging messages, inspired by SLF4J conventions.
         *
         * Provides methods for different log levels and supports printf-style formatting.
         * Concrete implementations handle the actual logging mechanism and formatting details.
         */
        class ILogger
        {
        public:
            virtual ~ILogger() = default;

            // Define log levels within the interface scope
            typedef enum
            {
                TRACE_LEVEL,
                DEBUG_LEVEL,
                INFO_LEVEL,
                WARN_LEVEL,
                ERROR_LEVEL
            } LogLevel;

// Printf-style formatted logging methods
// The __attribute__ provides compile-time format string checking on GCC/Clang.
#ifdef __GNUC__
            __attribute__((format(printf, 2, 3)))
#endif
            virtual void
            trace(const char *format, ...) = 0;

#ifdef __GNUC__
            __attribute__((format(printf, 2, 3)))
#endif
            virtual void
            debug(const char *format, ...) = 0;

#ifdef __GNUC__
            __attribute__((format(printf, 2, 3)))
#endif
            virtual void
            info(const char *format, ...) = 0;

#ifdef __GNUC__
            __attribute__((format(printf, 2, 3)))
#endif
            virtual void
            warn(const char *format, ...) = 0;

#ifdef __GNUC__
            __attribute__((format(printf, 2, 3)))
#endif
            virtual void
            error(const char *format, ...) = 0;
        };
    } // namespace api
} // namespace soc
