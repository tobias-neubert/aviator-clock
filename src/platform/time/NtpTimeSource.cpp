#include "platform/time/NtpTimeSource.h"

#include <cstdlib>
#include <sys/time.h>

#if __has_include(<esp_sntp.h>)
#include <esp_sntp.h>
#define AVIATOR_ESP_SNTP_AVAILABLE 1
#endif

namespace aviator::platform::time {

NtpTimeSource::NtpTimeSource(TimeSupplierFunction customTimeSupplier)
    : _customTimeSupplier(customTimeSupplier) {
}

NtpTimeSource::~NtpTimeSource() {
    stop();
}

bool NtpTimeSource::initialize(const NtpConfiguration& configuration) {
    if (configuration.timezone != nullptr && configuration.timezone[0] != '\0') {
        _activeTimezone = configuration.timezone;
    }

    // Configure the POSIX TZ environment variable so localtime_r applies daylight saving adjustments
    setenv("TZ", _activeTimezone, 1);
    tzset();

#if defined(AVIATOR_ESP_SNTP_AVAILABLE)
    if (esp_sntp_enabled()) {
        esp_sntp_stop();
    }

    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, configuration.primaryServer);
    esp_sntp_setservername(1, configuration.secondaryServer);
    esp_sntp_setservername(2, configuration.tertiaryServer);
    esp_sntp_init();
#endif

    _isInitialized = true;
    return true;
}

void NtpTimeSource::stop() {
#if defined(AVIATOR_ESP_SNTP_AVAILABLE)
    if (esp_sntp_enabled()) {
        esp_sntp_stop();
    }
#endif
    _isInitialized = false;
}

uint64_t NtpTimeSource::now() {
    if (!isSynchronized()) {
        return 0;
    }

    if (_customTimeSupplier != nullptr) {
        return static_cast<uint64_t>(_customTimeSupplier(nullptr)) * 1000ULL;
    }

    struct timeval currentTimeValue{};
    gettimeofday(&currentTimeValue, nullptr);

    return (static_cast<uint64_t>(currentTimeValue.tv_sec) * 1000ULL) +
           (static_cast<uint64_t>(currentTimeValue.tv_usec) / 1000ULL);
}

bool NtpTimeSource::asTimeComponents(domain::TimeComponents& components) {
    time_t rawEpochSeconds = 0;
    if (_customTimeSupplier != nullptr) {
        rawEpochSeconds = _customTimeSupplier(nullptr);
    } else {
        rawEpochSeconds = ::time(nullptr);
    }

    struct tm brokenDownTime{};
    if (localtime_r(&rawEpochSeconds, &brokenDownTime) == nullptr) {
        return false;
    }

    // Unsynchronized ESP32 starts at year 1970 (tm_year = 70).
    // An active NTP synchronization updates the hardware clock to current real time.
    if (brokenDownTime.tm_year < (MINIMUM_VALID_YEAR - 1900)) {
        return false;
    }

    components.hours = static_cast<uint8_t>(brokenDownTime.tm_hour);
    components.minutes = static_cast<uint8_t>(brokenDownTime.tm_min);
    components.seconds = static_cast<uint8_t>(brokenDownTime.tm_sec);
    return true;
}

bool NtpTimeSource::isSynchronized() const {
    time_t rawEpochSeconds = 0;
    if (_customTimeSupplier != nullptr) {
        rawEpochSeconds = _customTimeSupplier(nullptr);
    } else {
        rawEpochSeconds = ::time(nullptr);
    }

    struct tm brokenDownTime{};
    if (localtime_r(&rawEpochSeconds, &brokenDownTime) == nullptr) {
        return false;
    }

    return brokenDownTime.tm_year >= (MINIMUM_VALID_YEAR - 1900);
}

void NtpTimeSource::setTimeSupplier(TimeSupplierFunction customTimeSupplier) {
    _customTimeSupplier = customTimeSupplier;
}

const char* NtpTimeSource::getTimezone() const {
    return _activeTimezone;
}

} // namespace aviator::platform::time
