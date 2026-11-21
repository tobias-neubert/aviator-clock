#pragma once

#include <gmock/gmock.h>
#include "domain/ClockConfigurationApi.h"

namespace aviator::test {

class MockClockConfigurationApi : public domain::ClockConfigurationApi {
public:
    MOCK_METHOD(domain::Alarm, getAlarm, (), (const, override));
    MOCK_METHOD(bool, updateAlarm, (const domain::Alarm& alarm), (override));
    MOCK_METHOD(domain::SystemStatus, getSystemStatus, (), (const, override));
    MOCK_METHOD(void, previewSoundTrack, (uint16_t soundTrackId), (override));
    MOCK_METHOD(void, stopAudioPreview, (), (override));
};

} // namespace aviator::test
