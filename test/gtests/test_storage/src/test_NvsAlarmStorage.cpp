#include <gtest/gtest.h>

#include "platform/storage/NvsAlarmStorage.h"
#include "Preferences.h"

using namespace aviator::platform::storage;
using namespace aviator::domain;

class NvsAlarmStorageTest : public ::testing::Test {
protected:
    void SetUp() override {
        Preferences::resetStorage();
    }

    void TearDown() override {
        Preferences::resetStorage();
    }

    NvsAlarmStorage storage{"test_alarm"};
};

TEST_F(NvsAlarmStorageTest, LoadAlarm_WhenStorageIsEmpty_ReturnsDefaultAlarm) {
    const Alarm loadedAlarm = storage.loadAlarm();

    EXPECT_EQ(loadedAlarm.time.hours, 0);
    EXPECT_EQ(loadedAlarm.time.minutes, 0);
    EXPECT_EQ(loadedAlarm.time.seconds, 0);
    EXPECT_EQ(loadedAlarm.soundTrackId, 1);
    EXPECT_FALSE(loadedAlarm.enabled);
}

TEST_F(NvsAlarmStorageTest, SaveAlarm_AndLoadAlarm_PersistsAndRestoresValues) {
    const Alarm alarmToSave{Time{7, 45, 0}, 4, true};

    const bool saveResult = storage.saveAlarm(alarmToSave);
    EXPECT_TRUE(saveResult);

    const Alarm loadedAlarm = storage.loadAlarm();
    EXPECT_EQ(loadedAlarm, alarmToSave);
}

TEST_F(NvsAlarmStorageTest, SaveAlarm_UpdatesExistingAlarmValues) {
    const Alarm initialAlarm{Time{6, 30, 0}, 2, false};
    EXPECT_TRUE(storage.saveAlarm(initialAlarm));
    EXPECT_EQ(storage.loadAlarm(), initialAlarm);

    const Alarm updatedAlarm{Time{18, 15, 0}, 5, true};
    EXPECT_TRUE(storage.saveAlarm(updatedAlarm));
    EXPECT_EQ(storage.loadAlarm(), updatedAlarm);
}

TEST_F(NvsAlarmStorageTest, SaveAlarm_WhenWriteFails_ReturnsFalse) {
    Preferences::setSimulateWriteFailure(true);

    const Alarm alarmToSave{Time{8, 0, 0}, 1, true};
    EXPECT_FALSE(storage.saveAlarm(alarmToSave));
}

TEST_F(NvsAlarmStorageTest, LoadAlarm_WhenBeginFails_ReturnsDefaultAlarm) {
    // First save an alarm successfully
    const Alarm alarmToSave{Time{9, 30, 0}, 3, true};
    EXPECT_TRUE(storage.saveAlarm(alarmToSave));

    // Simulate begin failure (e.g. partition unmounted or locked)
    Preferences::setSimulateBeginFailure(true);
    const Alarm loadedAlarm = storage.loadAlarm();

    // Must gracefully fall back to default alarm
    EXPECT_EQ(loadedAlarm, Alarm{});
}

TEST_F(NvsAlarmStorageTest, SaveAlarm_WhenBeginFails_ReturnsFalse) {
    Preferences::setSimulateBeginFailure(true);

    const Alarm alarmToSave{Time{10, 0, 0}, 2, true};
    EXPECT_FALSE(storage.saveAlarm(alarmToSave));
}

TEST_F(NvsAlarmStorageTest, Clear_ErasesAllStoredAlarmSettings) {
    const Alarm alarmToSave{Time{12, 0, 0}, 6, true};
    EXPECT_TRUE(storage.saveAlarm(alarmToSave));
    EXPECT_EQ(storage.loadAlarm(), alarmToSave);

    EXPECT_TRUE(storage.clear());

    const Alarm loadedAlarmAfterClear = storage.loadAlarm();
    EXPECT_EQ(loadedAlarmAfterClear.time.hours, 0);
    EXPECT_EQ(loadedAlarmAfterClear.time.minutes, 0);
    EXPECT_EQ(loadedAlarmAfterClear.soundTrackId, 1);
    EXPECT_FALSE(loadedAlarmAfterClear.enabled);
}

TEST_F(NvsAlarmStorageTest, BoundaryValues_MaxHourMinuteAndTrackId) {
    const Alarm boundaryAlarm{Time{23, 59, 0}, 65535, true};
    EXPECT_TRUE(storage.saveAlarm(boundaryAlarm));

    const Alarm loadedAlarm = storage.loadAlarm();
    EXPECT_EQ(loadedAlarm.time.hours, 23);
    EXPECT_EQ(loadedAlarm.time.minutes, 59);
    EXPECT_EQ(loadedAlarm.soundTrackId, 65535);
    EXPECT_TRUE(loadedAlarm.enabled);
}

TEST_F(NvsAlarmStorageTest, CustomNamespace_IsIsolated) {
    NvsAlarmStorage storageNamespaceA{"ns_alpha"};
    NvsAlarmStorage storageNamespaceB{"ns_beta"};

    const Alarm alarmA{Time{6, 0, 0}, 1, true};
    const Alarm alarmB{Time{18, 0, 0}, 2, false};

    EXPECT_TRUE(storageNamespaceA.saveAlarm(alarmA));
    EXPECT_TRUE(storageNamespaceB.saveAlarm(alarmB));

    EXPECT_EQ(storageNamespaceA.loadAlarm(), alarmA);
    EXPECT_EQ(storageNamespaceB.loadAlarm(), alarmB);
}
