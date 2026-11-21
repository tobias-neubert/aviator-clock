#include "domain/AlarmCoordinator.h"

namespace aviator::domain {

AlarmCoordinator::AlarmCoordinator(AudioPlayer& audioPlayer, AlarmStorage& alarmStorage)
    : _audioPlayer(audioPlayer), _alarmStorage(alarmStorage) {}

void AlarmCoordinator::initialize() {
    _alarm = _alarmStorage.loadAlarm();
    _isRinging = false;
    _isDismissedForCurrentAlarmEvent = false;
    _lastTriggeredMinute = -1;
    _lastTriggeredHour = -1;
}

void AlarmCoordinator::update(const Time& currentTime) {
    if (!_alarm.enabled) {
        if (_isRinging) {
            stopAlarm();
        }
        _isDismissedForCurrentAlarmEvent = false;
        return;
    }

    const bool isMatchingAlarmTime = (currentTime.hours == _alarm.time.hours) &&
                                     (currentTime.minutes == _alarm.time.minutes);

    if (isMatchingAlarmTime) {
        if (!_isRinging && !_isDismissedForCurrentAlarmEvent) {
            _isRinging = true;
            _lastTriggeredMinute = static_cast<int8_t>(currentTime.minutes);
            _lastTriggeredHour = static_cast<int8_t>(currentTime.hours);
            _audioPlayer.playTrack(_alarm.soundTrackId);
        }
    } else {
        if (_isDismissedForCurrentAlarmEvent) {
            _isDismissedForCurrentAlarmEvent = false;
        }
        if (_isRinging) {
            stopAlarm();
        }
    }
}

void AlarmCoordinator::dismissAlarm() {
    if (_isRinging) {
        _isRinging = false;
        _isDismissedForCurrentAlarmEvent = true;
        _audioPlayer.stop();
    }
}

void AlarmCoordinator::stopAlarm() {
    _isRinging = false;
    _audioPlayer.stop();
}

void AlarmCoordinator::setAlarm(const Alarm& alarm) {
    _alarm = alarm;
    _alarmStorage.saveAlarm(_alarm);

    if (!_alarm.enabled && _isRinging) {
        stopAlarm();
    }
}

const Alarm& AlarmCoordinator::getAlarm() const {
    return _alarm;
}

bool AlarmCoordinator::isRinging() const {
    return _isRinging;
}

bool AlarmCoordinator::isDismissedForCurrentEvent() const {
    return _isDismissedForCurrentAlarmEvent;
}

} // namespace aviator::domain
