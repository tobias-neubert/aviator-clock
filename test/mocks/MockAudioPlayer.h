#pragma once

#include <gmock/gmock.h>
#include "domain/AudioPlayer.h"

namespace aviator::test {

class MockAudioPlayer : public domain::AudioPlayer {
public:
    MOCK_METHOD(void, playTrack, (uint16_t trackNumber), (override));
    MOCK_METHOD(void, stop, (), (override));
    MOCK_METHOD(void, setVolume, (uint8_t volume), (override));
};

} // namespace aviator::test
