#include <gtest/gtest.h>
#include "domain/ClockConfigurationService.h"
#include "FakeAlarmStorage.h"
#include "MockAudioPlayer.h"

using namespace aviator;
using namespace testing;

class ClockConfigurationServiceTest : public ::testing::Test {
protected:
    test::FakeAlarmStorage alarmStorage{domain::Alarm{domain::Time{6, 30, 0}, 2, true}};
    NiceMock<test::MockAudioPlayer> audioPlayer;
    domain::AlarmCoordinator alarmCoordinator{audioPlayer, alarmStorage};
    domain::OperatingStateMachine stateMachine{domain::OperatingMode::NormalRunning};

    void SetUp() override {
        alarmCoordinator.initialize();
    }
};

TEST_F(ClockConfigurationServiceTest, GetAlarm_ReturnsAlarmFromCoordinator) {
    domain::ClockConfigurationService service{alarmCoordinator, audioPlayer, stateMachine};

    domain::Alarm alarm = service.getAlarm();
    EXPECT_EQ(alarm.time.hours, 6);
    EXPECT_EQ(alarm.time.minutes, 30);
    EXPECT_EQ(alarm.soundTrackId, 2);
    EXPECT_TRUE(alarm.enabled);
}

TEST_F(ClockConfigurationServiceTest, UpdateAlarm_ValidAlarm_UpdatesCoordinatorAndStorage) {
    domain::ClockConfigurationService service{alarmCoordinator, audioPlayer, stateMachine};

    domain::Alarm newAlarm{domain::Time{8, 15, 0}, 4, false};
    EXPECT_TRUE(service.updateAlarm(newAlarm));

    domain::Alarm retrievedAlarm = service.getAlarm();
    EXPECT_EQ(retrievedAlarm.time.hours, 8);
    EXPECT_EQ(retrievedAlarm.time.minutes, 15);
    EXPECT_EQ(retrievedAlarm.soundTrackId, 4);
    EXPECT_FALSE(retrievedAlarm.enabled);
}

TEST_F(ClockConfigurationServiceTest, UpdateAlarm_InvalidAlarm_ReturnsFalseAndDoesNotUpdate) {
    domain::ClockConfigurationService service{alarmCoordinator, audioPlayer, stateMachine};

    domain::Alarm invalidTimeAlarm{domain::Time{25, 0, 0}, 1, true};
    EXPECT_FALSE(service.updateAlarm(invalidTimeAlarm));

    domain::Alarm invalidTrackAlarm{domain::Time{8, 0, 0}, 0, true};
    EXPECT_FALSE(service.updateAlarm(invalidTrackAlarm));

    // Stored alarm should remain unchanged
    domain::Alarm retrievedAlarm = service.getAlarm();
    EXPECT_EQ(retrievedAlarm.time.hours, 6);
    EXPECT_EQ(retrievedAlarm.time.minutes, 30);
}

TEST_F(ClockConfigurationServiceTest, GetSystemStatus_CombinesDomainStates) {
    bool wifiConnected = false;
    bool timeSynchronized = true;

    domain::ClockConfigurationService service{
        alarmCoordinator,
        audioPlayer,
        stateMachine,
        [&wifiConnected]() { return wifiConnected; },
        [&timeSynchronized]() { return timeSynchronized; }
    };

    domain::SystemStatus status = service.getSystemStatus();
    EXPECT_EQ(status.operatingMode, domain::OperatingMode::NormalRunning);
    EXPECT_FALSE(status.isWifiConnected);
    EXPECT_TRUE(status.isTimeSynchronized);
    EXPECT_FALSE(status.isAlarmRinging);

    wifiConnected = true;
    stateMachine.enterSetupMode();

    domain::SystemStatus updatedStatus = service.getSystemStatus();
    EXPECT_EQ(updatedStatus.operatingMode, domain::OperatingMode::SetupMode);
    EXPECT_TRUE(updatedStatus.isWifiConnected);
}

TEST_F(ClockConfigurationServiceTest, PreviewSoundTrack_DelegatesToAudioPlayer) {
    domain::ClockConfigurationService service{alarmCoordinator, audioPlayer, stateMachine};

    EXPECT_CALL(audioPlayer, playTrack(3)).Times(1);
    service.previewSoundTrack(3);
}

TEST_F(ClockConfigurationServiceTest, StopAudioPreview_DelegatesToAudioPlayer) {
    domain::ClockConfigurationService service{alarmCoordinator, audioPlayer, stateMachine};

    EXPECT_CALL(audioPlayer, stop()).Times(1);
    service.stopAudioPreview();
}
