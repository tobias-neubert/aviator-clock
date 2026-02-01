#ifndef MOCK_LOGGER_H
#define MOCK_LOGGER_H

#include <gmock/gmock.h>
#include <soc/api/Logger.h>
#include <cstdarg> // For va_list, va_start, va_end
#include <cstdio>  // For vsnprintf

namespace soc {
namespace api {
namespace testing {

class MockLogger : public soc::api::Logger {
public:
    // The constructor needs to call the base class constructor (default)

    // Mock the protected virtual method that takes the formatted string
    MOCK_METHOD(void, _log_formatted, (LogLevel level, const char* tag, const char* formatted_message), (override));

public:
    // The public variadic methods will now call _log_formatted, so we don't need to mock them directly.
    // However, we still need to provide an implementation for them in the mock.
    // These implementations will simply forward to the real base class implementations, which in turn
    // will call the mocked _log_formatted.
    void i(const char* tag, const char* format, ...) override {
        va_list args;
        va_start(args, format);
        char buffer[256];
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        _log_formatted(LogLevel::INFO, tag, buffer);
    }
    void w(const char* tag, const char* format, ...) override {
        va_list args;
        va_start(args, format);
        char buffer[256];
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        _log_formatted(LogLevel::WARNING, tag, buffer);
    }
    void e(const char* tag, const char* format, ...) override {
        va_list args;
        va_start(args, format);
        char buffer[256];
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        _log_formatted(LogLevel::ERROR, tag, buffer);
    }
    void d(const char* tag, const char* format, ...) override {
        va_list args;
        va_start(args, format);
        char buffer[256];
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        _log_formatted(LogLevel::DEBUG, tag, buffer);
    }

};

} // namespace testing
} // namespace api
} // namespace soc

#endif // MOCK_LOGGER_H