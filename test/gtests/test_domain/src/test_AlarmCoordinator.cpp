#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <atomic>
#include <thread>

#include "domain/AlarmCoordinator.h"
#include "MockAudioPlayer.h"
#include "MockAlarmStorage.h"

using namespace aviator::domain;
using namespace aviator::test;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::_;

class AlarmCoordinatorTest : public ::testing::Test {
protected:
    NiceMock<MockAudioPlayer> audioPlayer;
    NiceMock<MockAlarmStorage> alarmStorage;
    AlarmCoordinator coordinator{audioPlayer, alarmStorage};
};

TEST_F(AlarmCoordinatorTest, Initialize_LoadsAlarmFromStorage) {
    const Alarm persistedAlarm(Time(6, 30, 0), 3, true);
    EXPECT_CALL(alarmStorage, loadAlarm()).WillOnce(Return(persistedAlarm));

    coordinator.initialize();

    EXPECT_EQ(coordinator.getAlarm(), persistedAlarm);
    EXPECT_FALSE(coordinator.isRinging());
}

TEST_F(AlarmCoordinatorTest, Update_DoesNotTrigger_WhenAlarmDisabled) {
    coordinator.setAlarm(Alarm(Time(7, 0, 0), 2, false));

    EXPECT_CALL(audioPlayer, playTrack(_)).Times(0);

    coordinator.update(Time(7, 0, 0));
    EXPECT_FALSE(coordinator.isRinging());
}

TEST_F(AlarmCoordinatorTest, Update_DoesNotTrigger_WhenTimeDoesNotMatch) {
    coordinator.setAlarm(Alarm(Time(7, 0, 0), 2, true));

    EXPECT_CALL(audioPlayer, playTrack(_)).Times(0);

    coordinator.update(Time(6, 59, 59));
    EXPECT_FALSE(coordinator.isRinging());
}

TEST_F(AlarmCoordinatorTest, Update_TriggersAudio_WhenTimeMatchesAndAlarmEnabled) {
    coordinator.setAlarm(Alarm(Time(7, 0, 0), 5, true));

    EXPECT_CALL(audioPlayer, playTrack(5)).Times(1);

    coordinator.update(Time(7, 0, 0));
    EXPECT_TRUE(coordinator.isRinging());

    // Subsequent updates within the same minute do not re-trigger playTrack
    coordinator.update(Time(7, 0, 15));
    coordinator.update(Time(7, 0, 30));
    EXPECT_TRUE(coordinator.isRinging());
}

TEST_F(AlarmCoordinatorTest, DismissAlarm_StopsAudio_AndPreventsRetriggerInSameMinute) {
    coordinator.setAlarm(Alarm(Time(8, 15, 0), 1, true));
    coordinator.update(Time(8, 15, 0));
    EXPECT_TRUE(coordinator.isRinging());

    EXPECT_CALL(audioPlayer, stop()).Times(1);
    coordinator.dismissAlarm();

    EXPECT_FALSE(coordinator.isRinging());
    EXPECT_TRUE(coordinator.isDismissedForCurrentEvent());

    // Should NOT re-trigger in the same minute
    EXPECT_CALL(audioPlayer, playTrack(_)).Times(0);
    coordinator.update(Time(8, 15, 10));
    EXPECT_FALSE(coordinator.isRinging());

    // Next minute resets dismissal flag
    coordinator.update(Time(8, 16, 0));
    EXPECT_FALSE(coordinator.isDismissedForCurrentEvent());
}

TEST_F(AlarmCoordinatorTest, DisablingAlarmWhileRinging_StopsAudioImmediately) {
    coordinator.setAlarm(Alarm(Time(7, 30, 0), 2, true));
    coordinator.update(Time(7, 30, 0));
    EXPECT_TRUE(coordinator.isRinging());

    EXPECT_CALL(audioPlayer, stop()).Times(1);
    coordinator.setAlarm(Alarm(Time(7, 30, 0), 2, false));

    EXPECT_FALSE(coordinator.isRinging());
}

TEST_F(AlarmCoordinatorTest, Update_StopsRinging_WhenMinutePassesWithoutDismiss) {
    coordinator.setAlarm(Alarm(Time(7, 30, 0), 2, true));
    coordinator.update(Time(7, 30, 0));
    EXPECT_TRUE(coordinator.isRinging());

    EXPECT_CALL(audioPlayer, stop()).Times(1);
    // Minute passed, alarm should stop automatically
    coordinator.update(Time(7, 31, 0));
    EXPECT_FALSE(coordinator.isRinging());
}

TEST_F(AlarmCoordinatorTest, SetAlarm_PersistsToStorage) {
    const Alarm newAlarm(Time(9, 45, 0), 4, true);
    EXPECT_CALL(alarmStorage, saveAlarm(newAlarm)).WillOnce(Return(true));

    coordinator.setAlarm(newAlarm);
    EXPECT_EQ(coordinator.getAlarm(), newAlarm);
}

TEST_F(AlarmCoordinatorTest, ConcurrentReadWrite_ThreadSafetyGauntlet) {
    std::atomic<bool> stopRunning{false};

    std::thread readerThread([&]() {
        uint8_t sec = 0;
        while (!stopRunning.load()) {
            coordinator.update(Time(7, 30, sec % 60));
            const Alarm current = coordinator.getAlarm();
            EXPECT_TRUE(current.time.isValid());
            sec++;
        }
    });

    std::thread writerThread([&]() {
        for (int i = 0; i < 500; ++i) {
            coordinator.setAlarm(Alarm(Time(i % 24, i % 60, 0), static_cast<uint16_t>((i % 5) + 1), (i % 2) == 0));
        }
    });

    std::thread dismissThread([&]() {
        while (!stopRunning.load()) {
            coordinator.dismissAlarm();
        }
    });

    writerThread.join();
    stopRunning.store(true);
    readerThread.join();
    dismissThread.join();

    EXPECT_TRUE(coordinator.getAlarm().time.isValid());
}
