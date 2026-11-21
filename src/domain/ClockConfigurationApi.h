#pragma once

#include <cstdint>
#include "domain/Alarm.h"
#include "domain/OperatingStateMachine.h"

namespace aviator::domain {

/**
 * @brief Aggregate system status structure exposed to the configuration portal.
 */
struct SystemStatus {
    OperatingMode operatingMode{OperatingMode::Initial};
    bool isWifiConnected{false};
    bool isTimeSynchronized{false};
    bool isAlarmRinging{false};

    bool operator==(const SystemStatus& other) const {
        return operatingMode == other.operatingMode &&
               isWifiConnected == other.isWifiConnected &&
               isTimeSynchronized == other.isTimeSynchronized &&
               isAlarmRinging == other.isAlarmRinging;
    }

    bool operator!=(const SystemStatus& other) const {
        return !(*this == other);
    }
};

/**
 * @brief Abstract inbound port providing query and configuration operations for the web portal.
 */
class ClockConfigurationApi {
public:
    virtual ~ClockConfigurationApi() = default;

    /**
     * @brief Gets the current alarm configuration.
     * @return Stored Alarm structure.
     */
    virtual Alarm getAlarm() const = 0;

    /**
     * @brief Updates and persists the alarm configuration.
     * @param alarm New alarm settings.
     * @return True if update and persistence succeeded, false otherwise.
     */
    virtual bool updateAlarm(const Alarm& alarm) = 0;

    /**
     * @brief Queries current high-level system, network, and operational status.
     * @return Current SystemStatus.
     */
    virtual SystemStatus getSystemStatus() const = 0;

    /**
     * @brief Plays a sound track preview on the sound module.
     * @param soundTrackId Identifier of the sound track.
     */
    virtual void previewSoundTrack(uint16_t soundTrackId) = 0;

    /**
     * @brief Stops sound track preview playback.
     */
    virtual void stopAudioPreview() = 0;
};

} // namespace aviator::domain
