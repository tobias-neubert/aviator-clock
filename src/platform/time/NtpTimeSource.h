#pragma once

#include <cstdint>
#include <ctime>

#include "domain/Time.h"
#include "domain/TimeSource.h"

namespace aviator::platform::time {

/**
 * @brief Configuration parameters for Network Time Protocol (SNTP) synchronization.
 */
struct NtpConfiguration {
    const char* timezone{"CET-1CEST,M3.5.0,M10.5.0/3"};
    const char* primaryServer{"pool.ntp.org"};
    const char* secondaryServer{"time.google.com"};
    const char* tertiaryServer{"time.windows.com"};
};

/**
 * @brief Platform adapter implementing domain::TimeSource via ESP-IDF SNTP and POSIX time API.
 * 
 * Encapsulates ESP-IDF SNTP client lifecycle management and provides POSIX wall-clock
 * time component translations with daylight saving time support.
 */
class NtpTimeSource : public domain::TimeSource {
public:
    using TimeSupplierFunction = time_t (*)(time_t*);

    // Unsynchronized ESP32 RTC starts at epoch 1970; a valid synchronized NTP time must be >= 2024
    static constexpr int MINIMUM_VALID_YEAR = 2024;

    explicit NtpTimeSource(TimeSupplierFunction customTimeSupplier = nullptr);
    ~NtpTimeSource() override;

    /**
     * @brief Initializes the POSIX timezone environment and configures the ESP-IDF SNTP daemon.
     * @param configuration NTP servers and POSIX timezone configuration string.
     * @return True if initialized successfully.
     */
    bool initialize(const NtpConfiguration& configuration = {});

    /**
     * @brief Stops the background SNTP daemon if active.
     */
    void stop();

    /**
     * @brief Returns the current epoch time in milliseconds.
     * @return Milliseconds since January 1, 1970 UTC, or 0 if time has not been synchronized yet.
     */
    uint64_t now() override;

    /**
     * @brief Translates current local wall-clock time into domain TimeComponents.
     * @param components Reference to be populated with current hours, minutes, and seconds.
     * @return True if time is valid and synchronized, false otherwise.
     */
    bool asTimeComponents(domain::TimeComponents& components) override;

    /**
     * @brief Checks whether the system time has been synchronized with an NTP time server.
     * @return True if current year is at or above MINIMUM_VALID_YEAR, false otherwise.
     */
    bool isSynchronized() const;

    /**
     * @brief Overrides the time supplier function for deterministic host unit testing.
     * @param customTimeSupplier Function pointer returning POSIX time_t epoch seconds.
     */
    void setTimeSupplier(TimeSupplierFunction customTimeSupplier);

    /**
     * @brief Returns the currently active POSIX timezone configuration string.
     */
    const char* getTimezone() const;

private:
    TimeSupplierFunction _customTimeSupplier{nullptr};
    const char* _activeTimezone{"CET-1CEST,M3.5.0,M10.5.0/3"};
    bool _isInitialized{false};
};

} // namespace aviator::platform::time
