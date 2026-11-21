#pragma once

#include <cstdarg>
#include <cstdio>

namespace aviator::platform::logging {

/**
 * @brief Diagnostic logging severity levels.
 */
enum class LogLevel {
    Error = 0,
    Warning = 1,
    Info = 2,
    Debug = 3
};

/**
 * @brief Portable logging utility supporting tag-based formatting across target and host.
 */
class Logger {
public:
    virtual ~Logger() = default;

    virtual void i(const char* tag, const char* format, ...) {
        va_list arguments;
        va_start(arguments, format);
        logFormatted(LogLevel::Info, tag, format, arguments);
        va_end(arguments);
    }

    virtual void w(const char* tag, const char* format, ...) {
        va_list arguments;
        va_start(arguments, format);
        logFormatted(LogLevel::Warning, tag, format, arguments);
        va_end(arguments);
    }

    virtual void e(const char* tag, const char* format, ...) {
        va_list arguments;
        va_start(arguments, format);
        logFormatted(LogLevel::Error, tag, format, arguments);
        va_end(arguments);
    }

    virtual void d(const char* tag, const char* format, ...) {
        va_list arguments;
        va_start(arguments, format);
        logFormatted(LogLevel::Debug, tag, format, arguments);
        va_end(arguments);
    }

    void setLogLevel(LogLevel level) {
        _minimumLogLevel = level;
    }

    LogLevel getLogLevel() const {
        return _minimumLogLevel;
    }

protected:
    virtual void logFormatted(LogLevel level, const char* tag, const char* format, va_list arguments) {
        if (static_cast<int>(level) <= static_cast<int>(_minimumLogLevel)) {
            printf("[%s] ", tag);
            vprintf(format, arguments);
            printf("\n");
        }
    }

    LogLevel _minimumLogLevel{LogLevel::Info};
};

} // namespace aviator::platform::logging
