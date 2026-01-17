#pragma once

#include <soc/api/ILogger.h>
#include <iostream>
#include <string>
#include <cstdarg>

namespace soc
{
  namespace mock
  {
    /**
     * @brief Mock logger implementation that prints to the standard console.
     */
    class MockLogger : public soc::api::ILogger
    {
    public:
      void trace(const char *format, ...) override
      {
        va_list args;
        va_start(args, format);
        log("TRACE", format, args);
        va_end(args);
      }

      void debug(const char *format, ...) override
      {
        va_list args;
        va_start(args, format);
        log("DEBUG", format, args);
        va_end(args);
      }

      void info(const char *format, ...) override
      {
        va_list args;
        va_start(args, format);
        log("INFO ", format, args);
        va_end(args);
      }

      void warn(const char *format, ...) override
      {
        va_list args;
        va_start(args, format);
        log("WARN ", format, args);
        va_end(args);
      }

      void error(const char *format, ...) override
      {
        va_list args;
        va_start(args, format);
        log("ERROR", format, args);
        va_end(args);
      }

    private:
      void log(const char *prefix, const char *format, va_list args)
      {
        std::cout << "[" << prefix << "] ";
        // A buffer to hold the formatted string
        char buffer[256];
        vsnprintf(buffer, sizeof(buffer), format, args);
        std::cout << buffer << std::endl;
      }
    };
  } // namespace mock
} // namespace soc
