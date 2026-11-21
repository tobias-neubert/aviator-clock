#include "domain/ClockConfigurationService.h"

namespace aviator::domain {

ClockConfigurationService::ClockConfigurationService(
    AlarmCoordinator& alarmCoordinator,
    AudioPlayer& audioPlayer,
    OperatingStateMachine& operatingStateMachine,
    StatusQueryFunction wifiStatusSupplier,
    StatusQueryFunction timeSyncStatusSupplier)
    : _alarmCoordinator(alarmCoordinator),
      _audioPlayer(audioPlayer),
      _operatingStateMachine(operatingStateMachine),
      _wifiStatusSupplier(std::move(wifiStatusSupplier)),
      _timeSyncStatusSupplier(std::move(timeSyncStatusSupplier)) {
}

Alarm ClockConfigurationService::getAlarm() const {
    return _alarmCoordinator.getAlarm();
}

bool ClockConfigurationService::updateAlarm(const Alarm& alarm) {
    if (!alarm.time.isValid() || alarm.soundTrackId == 0) {
        return false;
    }
    _alarmCoordinator.setAlarm(alarm);
    return true;
}

SystemStatus ClockConfigurationService::getSystemStatus() const {
    SystemStatus status;
    status.operatingMode = _operatingStateMachine.getCurrentMode();
    status.isAlarmRinging = _alarmCoordinator.isRinging();
    status.isWifiConnected = (_wifiStatusSupplier != nullptr) ? _wifiStatusSupplier() : false;
    status.isTimeSynchronized = (_timeSyncStatusSupplier != nullptr) ? _timeSyncStatusSupplier() : false;
    return status;
}

void ClockConfigurationService::previewSoundTrack(uint16_t soundTrackId) {
    _audioPlayer.playTrack(soundTrackId);
}

void ClockConfigurationService::stopAudioPreview() {
    _audioPlayer.stop();
}

void ClockConfigurationService::setWifiStatusSupplier(StatusQueryFunction supplier) {
    _wifiStatusSupplier = std::move(supplier);
}

void ClockConfigurationService::setTimeSyncStatusSupplier(StatusQueryFunction supplier) {
    _timeSyncStatusSupplier = std::move(supplier);
}

} // namespace aviator::domain
