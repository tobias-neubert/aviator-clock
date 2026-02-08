#include <gtest/gtest.h>
#include "aviator-clock/AlarmRepository.h"
#include "soc/mock/MockPersistentStorage.h"

using namespace testing;
using namespace aviator_clock;
using namespace soc::mock;

class AlarmRepositoryTest : public Test {
protected:
    MockPersistentStorage storage;
    AlarmRepository repo{storage};

    void SetUp() override {
        repo.begin();
    }
};

TEST_F(AlarmRepositoryTest, GetAlarm_ReturnsDefaultZero_WhenStorageEmpty) {
    Time t = repo.getAlarm(0);
    EXPECT_EQ(0, t.hours);
    EXPECT_EQ(0, t.minutes);
}

TEST_F(AlarmRepositoryTest, SetAndGetAlarm_Slot0) {
    Time t = {12, 30, 0};
    repo.setAlarm(0, t);
    
    Time retrieved = repo.getAlarm(0);
    EXPECT_EQ(12, retrieved.hours);
    EXPECT_EQ(30, retrieved.minutes);
}

TEST_F(AlarmRepositoryTest, SetAndGetAlarm_Slot7) {
    Time t = {23, 59, 0};
    repo.setAlarm(7, t);
    
    Time retrieved = repo.getAlarm(7);
    EXPECT_EQ(23, retrieved.hours);
    EXPECT_EQ(59, retrieved.minutes);
}

TEST_F(AlarmRepositoryTest, SlotIsolation) {
    repo.setAlarm(1, {10, 0, 0});
    repo.setAlarm(2, {11, 0, 0});
    
    EXPECT_EQ(10, repo.getAlarm(1).hours);
    EXPECT_EQ(11, repo.getAlarm(2).hours);
}

TEST_F(AlarmRepositoryTest, InvalidSlot_DoesNotCrash) {
    repo.getAlarm(8);
    repo.setAlarm(8, {0,0,0});
    // Should pass if no exception thrown
}
