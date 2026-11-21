#include "platform/storage/NvsAlarmStorage.h"

#include <Preferences.h>

namespace aviator::platform::storage {

namespace {
// NVS key names are kept concise (<= 15 characters limit enforced by ESP-IDF NVS)
constexpr const char* KEY_HOUR = "hour";
constexpr const char* KEY_MINUTE = "minute";
constexpr const char* KEY_ENABLED = "enabled";
constexpr const char* KEY_SOUND_TRACK_ID = "sound_track";
} // namespace

NvsAlarmStorage::NvsAlarmStorage(const char* namespaceName)
    : _namespaceName((namespaceName != nullptr && namespaceName[0] != '\0') ? namespaceName : DEFAULT_NAMESPACE) {
}

domain::Alarm NvsAlarmStorage::loadAlarm() {
    Preferences preferences;
    // Open Preferences namespace in read-only mode to prevent unintended partition allocation
    if (!preferences.begin(_namespaceName, true)) {
        // Fall back to default alarm values when the namespace does not yet exist
        return domain::Alarm{};
    }

    const uint8_t hours = preferences.getUChar(KEY_HOUR, 0);
    const uint8_t minutes = preferences.getUChar(KEY_MINUTE, 0);
    const bool enabled = preferences.getBool(KEY_ENABLED, false);
    const uint16_t soundTrackId = preferences.getUShort(KEY_SOUND_TRACK_ID, 1);

    preferences.end();

    return domain::Alarm{domain::Time{hours, minutes, 0}, soundTrackId, enabled};
}

bool NvsAlarmStorage::saveAlarm(const domain::Alarm& alarm) {
    Preferences preferences;
    // Open in read-write mode to write updated values
    if (!preferences.begin(_namespaceName, false)) {
        return false;
    }

    const size_t hourBytesWritten = preferences.putUChar(KEY_HOUR, alarm.time.hours);
    const size_t minuteBytesWritten = preferences.putUChar(KEY_MINUTE, alarm.time.minutes);
    const size_t enabledBytesWritten = preferences.putBool(KEY_ENABLED, alarm.enabled);
    const size_t soundTrackBytesWritten = preferences.putUShort(KEY_SOUND_TRACK_ID, alarm.soundTrackId);

    preferences.end();

    // Verify all four configuration fields were written successfully
    return (hourBytesWritten > 0 &&
            minuteBytesWritten > 0 &&
            enabledBytesWritten > 0 &&
            soundTrackBytesWritten > 0);
}

bool NvsAlarmStorage::clear() {
    Preferences preferences;
    if (!preferences.begin(_namespaceName, false)) {
        return false;
    }

    const bool cleared = preferences.clear();
    preferences.end();
    return cleared;
}

} // namespace aviator::platform::storage
