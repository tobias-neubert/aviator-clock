#pragma once

#include "domain/Alarm.h"
#include "domain/AlarmStorage.h"

namespace aviator::test {

/**
 * @brief In-memory test double (Fake) for domain::AlarmStorage.
 */
class FakeAlarmStorage : public domain::AlarmStorage {
public:
    explicit FakeAlarmStorage(const domain::Alarm& initialAlarm = domain::Alarm{})
        : _alarm(initialAlarm) {}

    domain::Alarm loadAlarm() override {
        return _alarm;
    }

    bool saveAlarm(const domain::Alarm& alarm) override {
        if (_simulateSaveFailure) {
            return false;
        }
        _alarm = alarm;
        _saveCount++;
        return true;
    }

    void setAlarm(const domain::Alarm& alarm) {
        _alarm = alarm;
    }

    void setSimulateSaveFailure(bool simulateFailure) {
        _simulateSaveFailure = simulateFailure;
    }

    size_t getSaveCount() const {
        return _saveCount;
    }

private:
    domain::Alarm _alarm{};
    bool _simulateSaveFailure{false};
    size_t _saveCount{0};
};

} // namespace aviator::test
