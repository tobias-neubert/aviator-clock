#pragma once

#include "domain/Alarm.h"

namespace aviator::domain {

/**
 * @brief Abstract outbound port for persisting and loading alarm settings.
 */
class AlarmStorage {
public:
    virtual ~AlarmStorage() = default;

    /**
     * @brief Loads the configured alarm from persistent non-volatile storage.
     * @return Loaded Alarm object, or default values if uninitialized.
     */
    virtual Alarm loadAlarm() = 0;

    /**
     * @brief Persists the given alarm to non-volatile storage.
     * @param alarm Alarm configuration to persist.
     * @return True if successfully saved, false otherwise.
     */
    virtual bool saveAlarm(const Alarm& alarm) = 0;
};

} // namespace aviator::domain
