#pragma once

#include <functional>
#include "domain/AlarmCoordinator.h"
#include "domain/AudioPlayer.h"
#include "domain/ClockConfigurationApi.h"
#include "domain/OperatingStateMachine.h"

namespace aviator::domain {

/**
 * @brief Domain service implementing ClockConfigurationApi by coordinating AlarmCoordinator,
 * AudioPlayer, and OperatingStateMachine.
 */
class ClockConfigurationService : public ClockConfigurationApi {
public:
    using StatusQueryFunction = std::function<bool()>;

    ClockConfigurationService(
        AlarmCoordinator& alarmCoordinator,
        AudioPlayer& audioPlayer,
        OperatingStateMachine& operatingStateMachine,
        StatusQueryFunction wifiStatusSupplier = nullptr,
        StatusQueryFunction timeSyncStatusSupplier = nullptr);

    ~ClockConfigurationService() override = default;

    Alarm getAlarm() const override;
    bool updateAlarm(const Alarm& alarm) override;
    SystemStatus getSystemStatus() const override;
    void previewSoundTrack(uint16_t soundTrackId) override;
    void stopAudioPreview() override;

    void setWifiStatusSupplier(StatusQueryFunction supplier);
    void setTimeSyncStatusSupplier(StatusQueryFunction supplier);

private:
    AlarmCoordinator& _alarmCoordinator;
    AudioPlayer& _audioPlayer;
    OperatingStateMachine& _operatingStateMachine;
    StatusQueryFunction _wifiStatusSupplier;
    StatusQueryFunction _timeSyncStatusSupplier;
};

} // namespace aviator::domain
