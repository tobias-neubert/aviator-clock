#include "domain/AlarmCoordinator.h"

namespace aviator::domain {

AlarmCoordinator::AlarmCoordinator(AudioPlayer& audioPlayer, AlarmStorage& alarmStorage)
    : _audioPlayer(audioPlayer), _alarmStorage(alarmStorage) {}

void AlarmCoordinator::initialize() {
    const Alarm loadedAlarm = _alarmStorage.loadAlarm();
    _alarm = loadedAlarm;
    _alarmSequence.store(0, std::memory_order_release);
    _isRinging.store(false, std::memory_order_release);
    _isDismissedForCurrentAlarmEvent.store(false, std::memory_order_release);
    _lastTriggeredMinute.store(-1, std::memory_order_release);
    _lastTriggeredHour.store(-1, std::memory_order_release);
}

void AlarmCoordinator::update(const Time& currentTime) {
    const Alarm currentAlarm = getAlarm();

    if (!currentAlarm.enabled) {
        if (_isRinging.load(std::memory_order_acquire)) {
            stopAlarm();
        }
        _isDismissedForCurrentAlarmEvent.store(false, std::memory_order_release);
        return;
    }

    const bool isMatchingAlarmTime = (currentTime.hours == currentAlarm.time.hours) &&
                                     (currentTime.minutes == currentAlarm.time.minutes);

    if (isMatchingAlarmTime) {
        if (!_isRinging.load(std::memory_order_acquire) &&
            !_isDismissedForCurrentAlarmEvent.load(std::memory_order_acquire)) {
            _isRinging.store(true, std::memory_order_release);
            _lastTriggeredMinute.store(static_cast<int8_t>(currentTime.minutes), std::memory_order_release);
            _lastTriggeredHour.store(static_cast<int8_t>(currentTime.hours), std::memory_order_release);
            _audioPlayer.playTrack(currentAlarm.soundTrackId);
        }
    } else {
        if (_isDismissedForCurrentAlarmEvent.load(std::memory_order_acquire)) {
            _isDismissedForCurrentAlarmEvent.store(false, std::memory_order_release);
        }
        if (_isRinging.load(std::memory_order_acquire)) {
            stopAlarm();
        }
    }
}

void AlarmCoordinator::dismissAlarm() {
    bool expectedRinging = true;
    if (_isRinging.compare_exchange_strong(expectedRinging, false, std::memory_order_acq_rel)) {
        _isDismissedForCurrentAlarmEvent.store(true, std::memory_order_release);
        _audioPlayer.stop();
    }
}

void AlarmCoordinator::stopAlarm() {
    _isRinging.store(false, std::memory_order_release);
    _audioPlayer.stop();
}

void AlarmCoordinator::setAlarm(const Alarm& alarm) {
    while (_writerLock.exchange(true, std::memory_order_acquire)) {
        // Spin wait (only in the rare event of concurrent write calls)
    }

    const uint32_t seq = _alarmSequence.load(std::memory_order_relaxed);
    _alarmSequence.store(seq + 1, std::memory_order_release);

    _alarm = alarm;

    _alarmSequence.store(seq + 2, std::memory_order_release);
    _writerLock.store(false, std::memory_order_release);

    _alarmStorage.saveAlarm(alarm);

    if (!alarm.enabled && _isRinging.load(std::memory_order_acquire)) {
        stopAlarm();
    }
}

Alarm AlarmCoordinator::getAlarm() const {
    Alarm snapshot;
    uint32_t seqBefore = 0;
    do {
        seqBefore = _alarmSequence.load(std::memory_order_acquire);
        while (seqBefore & 1) {
            seqBefore = _alarmSequence.load(std::memory_order_acquire);
        }
        snapshot = _alarm;
    } while (_alarmSequence.load(std::memory_order_acquire) != seqBefore);

    return snapshot;
}

bool AlarmCoordinator::isRinging() const {
    return _isRinging.load(std::memory_order_acquire);
}

bool AlarmCoordinator::isDismissedForCurrentEvent() const {
    return _isDismissedForCurrentAlarmEvent.load(std::memory_order_acquire);
}

} // namespace aviator::domain
