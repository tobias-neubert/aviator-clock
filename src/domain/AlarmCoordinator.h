#pragma once

#include <atomic>
#include <cstdint>
#include "domain/Alarm.h"
#include "domain/AlarmStorage.h"
#include "domain/AudioPlayer.h"
#include "domain/Time.h"

namespace aviator::domain {

/**
 * @brief Coordinates alarm evaluation against real-time clock inputs, triggering
 * audio playback upon matching alarm time, and handling alarm dismissal.
 * 
 * Thread-safe for concurrent evaluation, user dismissals, and web configuration updates
 * across multiple FreeRTOS tasks without dynamic memory allocation or blocking mutexes.
 */
class AlarmCoordinator {
public:
    AlarmCoordinator(AudioPlayer& audioPlayer, AlarmStorage& alarmStorage);

    void initialize();

    void update(const Time& currentTime);

    void dismissAlarm();
    void stopAlarm();

    void setAlarm(const Alarm& alarm);
    Alarm getAlarm() const;

    bool isRinging() const;
    bool isDismissedForCurrentEvent() const;

private:
    AudioPlayer& _audioPlayer;
    AlarmStorage& _alarmStorage;

    // Sequence lock for lock-free, zero-allocation concurrent reads/writes of _alarm
    mutable std::atomic<uint32_t> _alarmSequence{0};
    mutable std::atomic<bool> _writerLock{false};
    Alarm _alarm{};

    std::atomic<bool> _isRinging{false};
    std::atomic<bool> _isDismissedForCurrentAlarmEvent{false};
    std::atomic<int8_t> _lastTriggeredMinute{-1};
    std::atomic<int8_t> _lastTriggeredHour{-1};
};

} // namespace aviator::domain
