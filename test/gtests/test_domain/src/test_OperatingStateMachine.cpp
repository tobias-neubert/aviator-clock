#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "domain/OperatingStateMachine.h"
#include "MockSystemGateway.h"

using namespace aviator::domain;
using namespace aviator::test;
using ::testing::NiceMock;

class MockOperatingModeObserver : public OperatingModeObserver {
public:
    MOCK_METHOD(void, onOperatingModeChanged, (OperatingMode previousMode, OperatingMode newMode), (override));
};

class OperatingStateMachineTest : public ::testing::Test {
protected:
    OperatingStateMachine stateMachine;
    MockOperatingModeObserver observer;
    NiceMock<MockSystemGateway> systemGateway;

    void SetUp() override {
        stateMachine.setObserver(&observer);
        stateMachine.setSystemGateway(&systemGateway);
    }
};

TEST_F(OperatingStateMachineTest, InitialState_IsInitial) {
    EXPECT_EQ(stateMachine.getCurrentMode(), OperatingMode::Initial);
    EXPECT_TRUE(stateMachine.isInitial());
    EXPECT_FALSE(stateMachine.isNormalRunning());
    EXPECT_FALSE(stateMachine.isSetupMode());
}

TEST_F(OperatingStateMachineTest, OnStartupCompleted_TransitionsFromInitialToNormalRunning) {
    EXPECT_CALL(observer, onOperatingModeChanged(OperatingMode::Initial, OperatingMode::NormalRunning)).Times(1);

    stateMachine.onStartupCompleted();

    EXPECT_EQ(stateMachine.getCurrentMode(), OperatingMode::NormalRunning);
    EXPECT_TRUE(stateMachine.isNormalRunning());
    EXPECT_FALSE(stateMachine.isInitial());
}

TEST_F(OperatingStateMachineTest, OnStartupCompleted_WhenAlreadyRunning_DoesNotRetrigger) {
    EXPECT_CALL(observer, onOperatingModeChanged(OperatingMode::Initial, OperatingMode::NormalRunning)).Times(1);
    stateMachine.onStartupCompleted();

    // Second call should not transition or notify
    stateMachine.onStartupCompleted();
    EXPECT_TRUE(stateMachine.isNormalRunning());
}

TEST_F(OperatingStateMachineTest, OnSetupButtonLongPressed_TogglesBetweenNormalAndSetupMode) {
    stateMachine.onStartupCompleted();

    EXPECT_CALL(observer, onOperatingModeChanged(OperatingMode::NormalRunning, OperatingMode::SetupMode)).Times(1);
    stateMachine.onSetupButtonLongPressed();
    EXPECT_TRUE(stateMachine.isSetupMode());

    EXPECT_CALL(observer, onOperatingModeChanged(OperatingMode::SetupMode, OperatingMode::NormalRunning)).Times(1);
    stateMachine.onSetupButtonLongPressed();
    EXPECT_TRUE(stateMachine.isNormalRunning());
}

TEST_F(OperatingStateMachineTest, EnterAndExitSetupMode) {
    stateMachine.onStartupCompleted();

    EXPECT_CALL(observer, onOperatingModeChanged(OperatingMode::NormalRunning, OperatingMode::SetupMode)).Times(1);
    stateMachine.enterSetupMode();
    EXPECT_TRUE(stateMachine.isSetupMode());

    EXPECT_CALL(observer, onOperatingModeChanged(OperatingMode::SetupMode, OperatingMode::NormalRunning)).Times(1);
    stateMachine.exitSetupMode();
    EXPECT_TRUE(stateMachine.isNormalRunning());
}

TEST_F(OperatingStateMachineTest, OnSetupButtonFactoryReset_TriggersCredentialsResetAndRestart) {
    EXPECT_CALL(systemGateway, resetWifiCredentials()).Times(1);
    EXPECT_CALL(systemGateway, restart()).Times(1);

    stateMachine.onSetupButtonFactoryReset();
}
