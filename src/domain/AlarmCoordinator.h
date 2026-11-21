#pragma once

#include "domain/Alarm.h"
#include "domain/AlarmStorage.h"
#include "domain/AudioPlayer.h"
#include "domain/Time.h"

namespace aviator::domain {

/**
 * @brief Coordinates alarm evaluation against real-time clock inputs, triggering
 * audio playback upon matching alarm time, and handling alarm dismissal.
 */
class AlarmCoordinator {
public:
    AlarmCoordinator(AudioPlayer& audioPlayer, AlarmStorage& alarmStorage);

    void initialize();

    void update(const Time& currentTime);

    void dismissAlarm();
    void stopAlarm();

    void setAlarm(const Alarm& alarm);
    const Alarm& getAlarm() const;

    bool isRinging() const;
    bool isDismissedForCurrentEvent() const;

private:
    AudioPlayer& _audioPlayer;
    AlarmStorage& _alarmStorage;

    Alarm _alarm;
    bool _isRinging{false};
    bool _isDismissedForCurrentAlarmEvent{false};
    int8_t _lastTriggeredMinute{-1};
    int8_t _lastTriggeredHour{-1};
};

} // namespace aviator::domain
