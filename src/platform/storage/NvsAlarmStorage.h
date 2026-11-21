#pragma once

#include "domain/Alarm.h"
#include "domain/AlarmStorage.h"

namespace aviator::platform::storage {

/**
 * @brief Platform adapter implementing domain::AlarmStorage using ESP32 Preferences (NVS).
 * 
 * Directly loads and persists alarm configuration (hours, minutes, sound track ID, and enabled status)
 * into ESP32 non-volatile storage without intermediate legacy abstraction layers.
 */
class NvsAlarmStorage : public domain::AlarmStorage {
public:
    static constexpr const char* DEFAULT_NAMESPACE = "alarm";

    explicit NvsAlarmStorage(const char* namespaceName = DEFAULT_NAMESPACE);
    ~NvsAlarmStorage() override = default;

    /**
     * @brief Loads the persisted alarm from NVS.
     * @return Loaded Alarm object, or default values if uninitialized or opening fails.
     */
    domain::Alarm loadAlarm() override;

    /**
     * @brief Persists the given alarm to NVS.
     * @param alarm Alarm configuration to persist.
     * @return True if all fields were successfully written, false otherwise.
     */
    bool saveAlarm(const domain::Alarm& alarm) override;

    /**
     * @brief Clears all persisted alarm settings from NVS.
     * @return True if successfully cleared, false otherwise.
     */
    bool clear();

private:
    const char* _namespaceName;
};

} // namespace aviator::platform::storage
