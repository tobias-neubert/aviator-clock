#pragma once

#include <gmock/gmock.h>
#include <soc/api/ILogger.h>

namespace soc
{
    namespace testing
    {
        class LoggerMock : public soc::api::ILogger
        {
        public:
            // --- Mock-bare "Proxy"-Methoden ---
            // Wir mocken diese einfachen Methoden anstelle der variadischen Originale.
            MOCK_METHOD(void, log_trace, (const char *format), ());
            MOCK_METHOD(void, log_debug, (const char *format), ());
            MOCK_METHOD(void, log_info, (const char *format), ());
            MOCK_METHOD(void, log_warn, (const char *format), ());
            MOCK_METHOD(void, log_error, (const char *format), ());

            // --- Implementierung des ILogger-Interfaces ---
            // Diese Methoden werden vom Code aufgerufen, leiten aber den Anruf
            // an unsere mock-baren Methoden weiter.
            // Wir ignorieren hier die variadischen Argumente für den Test.

#ifdef __GNUC__
            __attribute__((format(printf, 2, 3)))
#endif
            void
            trace(const char *format, ...) override
            {
                log_trace(format);
            }

#ifdef __GNUC__
            __attribute__((format(printf, 2, 3)))
#endif
            void
            debug(const char *format, ...) override
            {
                log_debug(format);
            }

#ifdef __GNUC__
            __attribute__((format(printf, 2, 3)))
#endif
            void
            info(const char *format, ...) override
            {
                log_info(format);
            }

#ifdef __GNUC__
            __attribute__((format(printf, 2, 3)))
#endif
            void
            warn(const char *format, ...) override
            {
                log_warn(format);
            }

#ifdef __GNUC__
            __attribute__((format(printf, 2, 3)))
#endif
            void
            error(const char *format, ...) override
            {
                log_error(format);
            }
        };
    } // namespace testing
} // namespace soc