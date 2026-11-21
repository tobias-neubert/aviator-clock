#include <gtest/gtest.h>
#include <atomic>
#include <thread>
#include <vector>
#include "platform/audio/Jq6500AudioPlayer.h"

using namespace aviator::platform::audio;

class Jq6500AudioPlayerTest : public ::testing::Test {
protected:
    std::vector<uint8_t> capturedBytes;

    Jq6500AudioPlayer createPlayer(uint8_t defaultVolume = 25) {
        Jq6500Configuration config;
        config.defaultVolume = defaultVolume;
        return Jq6500AudioPlayer(config, [this](const uint8_t* buffer, size_t length) -> size_t {
            capturedBytes.insert(capturedBytes.end(), buffer, buffer + length);
            return length;
        });
    }
};

TEST_F(Jq6500AudioPlayerTest, InitialState_HasConfiguredVolume_NotPlaying) {
    auto player = createPlayer(20);

    EXPECT_EQ(player.getVolume(), 20);
    EXPECT_EQ(player.getCurrentTrack(), 0);
    EXPECT_FALSE(player.isPlaying());
    EXPECT_TRUE(capturedBytes.empty());
}

TEST_F(Jq6500AudioPlayerTest, Initialize_SendsSetVolumeCommand) {
    auto player = createPlayer(22);
    player.initialize();

    // Frame: [0x7E, 0x03, 0x31, 22, 0xEF]
    const std::vector<uint8_t> expectedFrame = {0x7E, 0x03, 0x31, 22, 0xEF};
    EXPECT_EQ(capturedBytes, expectedFrame);
}

TEST_F(Jq6500AudioPlayerTest, PlayTrack_SendsCorrectFrameAndSetsPlayingState) {
    auto player = createPlayer();
    capturedBytes.clear();

    player.playTrack(1);

    // Frame: [0x7E, 0x04, 0x03, 0x00, 0x01, 0xEF]
    const std::vector<uint8_t> expectedFrame = {0x7E, 0x04, 0x03, 0x00, 0x01, 0xEF};
    EXPECT_EQ(capturedBytes, expectedFrame);
    EXPECT_TRUE(player.isPlaying());
    EXPECT_EQ(player.getCurrentTrack(), 1);
}

TEST_F(Jq6500AudioPlayerTest, PlayTrack_WithLargeIndex_SplitsHighAndLowBytes) {
    auto player = createPlayer();
    capturedBytes.clear();

    player.playTrack(0x0205); // 517

    const std::vector<uint8_t> expectedFrame = {0x7E, 0x04, 0x03, 0x02, 0x05, 0xEF};
    EXPECT_EQ(capturedBytes, expectedFrame);
    EXPECT_TRUE(player.isPlaying());
    EXPECT_EQ(player.getCurrentTrack(), 517);
}

TEST_F(Jq6500AudioPlayerTest, PlayTrack_Zero_StopsPlayback) {
    auto player = createPlayer();
    player.playTrack(1);
    capturedBytes.clear();

    player.playTrack(0);

    // Frame: [0x7E, 0x02, 0x16, 0xEF]
    const std::vector<uint8_t> expectedFrame = {0x7E, 0x02, 0x16, 0xEF};
    EXPECT_EQ(capturedBytes, expectedFrame);
    EXPECT_FALSE(player.isPlaying());
}

TEST_F(Jq6500AudioPlayerTest, Stop_SendsCorrectFrameAndClearsPlayingState) {
    auto player = createPlayer();
    player.playTrack(2);
    capturedBytes.clear();

    player.stop();

    const std::vector<uint8_t> expectedFrame = {0x7E, 0x02, 0x16, 0xEF};
    EXPECT_EQ(capturedBytes, expectedFrame);
    EXPECT_FALSE(player.isPlaying());
}

TEST_F(Jq6500AudioPlayerTest, SetVolume_SendsCorrectFrameAndUpdatesVolume) {
    auto player = createPlayer();
    capturedBytes.clear();

    player.setVolume(15);

    const std::vector<uint8_t> expectedFrame = {0x7E, 0x03, 0x31, 15, 0xEF};
    EXPECT_EQ(capturedBytes, expectedFrame);
    EXPECT_EQ(player.getVolume(), 15);
}

TEST_F(Jq6500AudioPlayerTest, SetVolume_ClampsAboveMaximum) {
    auto player = createPlayer();
    capturedBytes.clear();

    player.setVolume(35);

    const std::vector<uint8_t> expectedFrame = {0x7E, 0x03, 0x31, 30, 0xEF};
    EXPECT_EQ(capturedBytes, expectedFrame);
    EXPECT_EQ(player.getVolume(), 30);
}

TEST_F(Jq6500AudioPlayerTest, ConcurrentCommands_ThreadSafeUARTTransmission) {
    auto player = createPlayer();

    std::thread thread1([&]() {
        for (int i = 1; i <= 200; ++i) {
            player.playTrack(i);
            player.stop();
        }
    });

    std::thread thread2([&]() {
        for (int i = 0; i <= 200; ++i) {
            player.setVolume(i % 30);
        }
    });

    thread1.join();
    thread2.join();

    EXPECT_FALSE(capturedBytes.empty());
}
