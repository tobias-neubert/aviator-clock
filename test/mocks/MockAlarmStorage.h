#pragma once

#include <gmock/gmock.h>
#include "domain/AlarmStorage.h"

namespace aviator::test {

class MockAlarmStorage : public domain::AlarmStorage {
public:
    MOCK_METHOD(domain::Alarm, loadAlarm, (), (override));
    MOCK_METHOD(bool, saveAlarm, (const domain::Alarm& alarm), (override));
};

} // namespace aviator::test
