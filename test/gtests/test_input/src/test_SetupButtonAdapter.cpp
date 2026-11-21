#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "platform/input/SetupButtonAdapter.h"
#include "MockClockInputEvents.h"

using namespace aviator::platform::input;
using namespace aviator::test;
using ::testing::StrictMock;

class SetupButtonAdapterTest : public ::testing::Test {
protected:
    StrictMock<MockClockInputEvents> clockInputEvents;
    SetupButtonConfiguration configuration{
        15,    // pin
        true,  // activeLow
        50,    // debounceDurationMilliseconds
        3000,  // longPressThresholdMilliseconds
        10000, // factoryResetThresholdMilliseconds
        20     // pollingIntervalMilliseconds
    };

    SetupButtonAdapter adapter{clockInputEvents, configuration};
};

TEST_F(SetupButtonAdapterTest, InitialState_IsReleasedAndIdle) {
    EXPECT_EQ(adapter.getState(), ButtonState::Released);
    EXPECT_FALSE(adapter.isPressed());
    EXPECT_FALSE(adapter.isRunning());

    const auto& config = adapter.getConfiguration();
    EXPECT_EQ(config.pin, 15);
    EXPECT_TRUE(config.activeLow);
    EXPECT_EQ(config.debounceDurationMilliseconds, 50U);
    EXPECT_EQ(config.longPressThresholdMilliseconds, 3000U);
    EXPECT_EQ(config.factoryResetThresholdMilliseconds, 10000U);
    EXPECT_EQ(config.pollingIntervalMilliseconds, 20U);
}

TEST_F(SetupButtonAdapterTest, Debounce_FiltersOutBriefNoiseGlitch) {
    // Electrical contact spike lasting only 20 ms (< 50 ms debounce)
    adapter.update(true, 100);
    EXPECT_EQ(adapter.getState(), ButtonState::DebouncingPress);
    EXPECT_FALSE(adapter.isPressed());

    // Signal drops back to inactive before debounce expires
    adapter.update(false, 120);
    EXPECT_EQ(adapter.getState(), ButtonState::Released);
    EXPECT_FALSE(adapter.isPressed());

    // No events should be dispatched
    adapter.update(false, 200);
    EXPECT_EQ(adapter.getState(), ButtonState::Released);
}

TEST_F(SetupButtonAdapterTest, Debounce_RequiresContinuouslyStableActiveSignal) {
    // First bounce
    adapter.update(true, 100);
    EXPECT_EQ(adapter.getState(), ButtonState::DebouncingPress);

    // Chatter back to false at 120 ms
    adapter.update(false, 120);
    EXPECT_EQ(adapter.getState(), ButtonState::Released);

    // Second bounce: settles active at 130 ms
    adapter.update(true, 130);
    EXPECT_EQ(adapter.getState(), ButtonState::DebouncingPress);

    // After 40 ms (170 ms), still debouncing
    adapter.update(true, 170);
    EXPECT_EQ(adapter.getState(), ButtonState::DebouncingPress);
    EXPECT_FALSE(adapter.isPressed());

    // After 50 ms (180 ms), press is confirmed
    adapter.update(true, 180);
    EXPECT_EQ(adapter.getState(), ButtonState::Pressed);
    EXPECT_TRUE(adapter.isPressed());
}

TEST_F(SetupButtonAdapterTest, ShortPress_DispatchesOnSetupButtonPressed) {
    EXPECT_CALL(clockInputEvents, onSetupButtonPressed()).Times(1);

    // Press starts at t = 100 ms
    adapter.update(true, 100);
    // Debounce confirmed at t = 150 ms
    adapter.update(true, 150);
    EXPECT_EQ(adapter.getState(), ButtonState::Pressed);

    // Button released at t = 300 ms (held for 200 ms)
    adapter.update(false, 300);
    EXPECT_EQ(adapter.getState(), ButtonState::DebouncingRelease);
    EXPECT_TRUE(adapter.isPressed()); // Still considered pressed until release debounce settles

    // Release debounce completed at t = 350 ms
    adapter.update(false, 350);
    EXPECT_EQ(adapter.getState(), ButtonState::Released);
    EXPECT_FALSE(adapter.isPressed());
}

TEST_F(SetupButtonAdapterTest, ShortPress_MultipleConsecutivePresses) {
    EXPECT_CALL(clockInputEvents, onSetupButtonPressed()).Times(2);

    // First short press
    adapter.update(true, 100);
    adapter.update(true, 150);
    adapter.update(false, 300);
    adapter.update(false, 350);

    // Idle gap
    adapter.update(false, 500);

    // Second short press
    adapter.update(true, 600);
    adapter.update(true, 650);
    adapter.update(false, 800);
    adapter.update(false, 850);
}

TEST_F(SetupButtonAdapterTest, ReleaseContactBounce_HandledCleanlyWithoutEarlyTrigger) {
    EXPECT_CALL(clockInputEvents, onSetupButtonPressed()).Times(1);

    // Press confirmed
    adapter.update(true, 100);
    adapter.update(true, 150);

    // Release begins at t = 300 ms
    adapter.update(false, 300);
    EXPECT_EQ(adapter.getState(), ButtonState::DebouncingRelease);

    // Contact chatter: bounces active at t = 310 ms
    adapter.update(true, 310);
    EXPECT_EQ(adapter.getState(), ButtonState::Pressed);

    // Release begins again at t = 320 ms
    adapter.update(false, 320);
    EXPECT_EQ(adapter.getState(), ButtonState::DebouncingRelease);

    // Release completes at t = 370 ms
    adapter.update(false, 370);
    EXPECT_EQ(adapter.getState(), ButtonState::Released);
}

TEST_F(SetupButtonAdapterTest, LongPress_DispatchesOnSetupButtonLongPressed) {
    EXPECT_CALL(clockInputEvents, onSetupButtonLongPressed()).Times(1);

    // Button pressed at t = 100 ms
    adapter.update(true, 100);
    adapter.update(true, 150);
    EXPECT_EQ(adapter.getState(), ButtonState::Pressed);

    // Held for 3500 ms (t = 3600 ms)
    adapter.update(true, 2000);
    adapter.update(true, 3600);

    // Released at t = 3600 ms
    adapter.update(false, 3600);
    EXPECT_EQ(adapter.getState(), ButtonState::DebouncingRelease);

    // Release confirmed at t = 3650 ms
    adapter.update(false, 3650);
    EXPECT_EQ(adapter.getState(), ButtonState::Released);
}

TEST_F(SetupButtonAdapterTest, LongPress_AtExactThreshold_DispatchesOnSetupButtonLongPressed) {
    EXPECT_CALL(clockInputEvents, onSetupButtonLongPressed()).Times(1);

    // Button pressed at t = 100 ms
    adapter.update(true, 100);
    adapter.update(true, 150);

    // Released at t = 3100 ms (held duration: 3100 - 100 = 3000 ms)
    adapter.update(false, 3100);
    adapter.update(false, 3150);
    EXPECT_EQ(adapter.getState(), ButtonState::Released);
}

TEST_F(SetupButtonAdapterTest, PressHeldJustUnderLongPressThreshold_DispatchesShortPress) {
    EXPECT_CALL(clockInputEvents, onSetupButtonPressed()).Times(1);

    // Button pressed at t = 100 ms
    adapter.update(true, 100);
    adapter.update(true, 150);

    // Released at t = 3099 ms (held duration: 3099 - 100 = 2999 ms < 3000 ms)
    adapter.update(false, 3099);
    adapter.update(false, 3149);
    EXPECT_EQ(adapter.getState(), ButtonState::Released);
}

TEST_F(SetupButtonAdapterTest, FactoryReset_DispatchesImmediatelyAtTenSecondsWithoutWaitingForRelease) {
    EXPECT_CALL(clockInputEvents, onSetupButtonFactoryReset()).Times(1);

    // Button pressed at t = 100 ms
    adapter.update(true, 100);
    adapter.update(true, 150);

    // Hold up to 9999 ms duration (t = 10099 ms)
    adapter.update(true, 5000);
    adapter.update(true, 10099);

    // Exact 10000 ms duration reached (t = 10100 ms): Factory reset fires immediately!
    adapter.update(true, 10100);

    // Continued holding should not re-trigger factory reset or any other event
    adapter.update(true, 11000);
    adapter.update(true, 12000);

    // Button finally released: should NOT fire short or long press
    adapter.update(false, 12000);
    adapter.update(false, 12050);
    EXPECT_EQ(adapter.getState(), ButtonState::Released);
    EXPECT_FALSE(adapter.isPressed());
}

TEST_F(SetupButtonAdapterTest, InjectedPinReaderAndTimeSupplier_WorksWithParameterlessUpdate) {
    bool rawPinActive = false;
    uint32_t currentSimulatedTimestamp = 1000;

    adapter.setPinReader([&rawPinActive]() { return rawPinActive; });
    adapter.setTimeSupplier([&currentSimulatedTimestamp]() { return currentSimulatedTimestamp; });

    EXPECT_CALL(clockInputEvents, onSetupButtonPressed()).Times(1);

    // Press button
    rawPinActive = true;
    adapter.update(); // t = 1000 ms

    currentSimulatedTimestamp = 1050;
    adapter.update(); // Debounce confirmed at t = 1050 ms
    EXPECT_TRUE(adapter.isPressed());

    // Release button
    currentSimulatedTimestamp = 1200;
    rawPinActive = false;
    adapter.update(); // Release begins

    currentSimulatedTimestamp = 1250;
    adapter.update(); // Release debounced -> triggers onSetupButtonPressed
    EXPECT_FALSE(adapter.isPressed());
}

TEST_F(SetupButtonAdapterTest, ActiveHighConfiguration_InterpretsPositiveLogicCorrectly) {
    SetupButtonConfiguration activeHighConfig{
        15,    // pin
        false, // activeLow: false (Active-HIGH)
        50,
        3000,
        10000,
        20
    };
    SetupButtonAdapter activeHighAdapter{clockInputEvents, activeHighConfig};

    EXPECT_CALL(clockInputEvents, onSetupButtonPressed()).Times(1);

    // In active-high logic, true means active (HIGH)
    activeHighAdapter.update(true, 100);
    activeHighAdapter.update(true, 150);
    EXPECT_TRUE(activeHighAdapter.isPressed());

    activeHighAdapter.update(false, 300);
    activeHighAdapter.update(false, 350);
    EXPECT_FALSE(activeHighAdapter.isPressed());
}

TEST_F(SetupButtonAdapterTest, ZeroDebounceDurationConfiguration_TransitionsImmediately) {
    SetupButtonConfiguration zeroDebounceConfig{
        15,
        true,
        0, // debounceDurationMilliseconds = 0
        3000,
        10000,
        20
    };
    SetupButtonAdapter instantAdapter{clockInputEvents, zeroDebounceConfig};

    EXPECT_CALL(clockInputEvents, onSetupButtonPressed()).Times(1);

    // Immediately enters Pressed on first update
    instantAdapter.update(true, 100);
    EXPECT_EQ(instantAdapter.getState(), ButtonState::Pressed);
    EXPECT_TRUE(instantAdapter.isPressed());

    // Immediately releases and dispatches on release
    instantAdapter.update(false, 200);
    EXPECT_EQ(instantAdapter.getState(), ButtonState::Released);
    EXPECT_FALSE(instantAdapter.isPressed());
}

TEST_F(SetupButtonAdapterTest, Lifecycle_StartAndStop) {
    EXPECT_FALSE(adapter.isRunning());

    adapter.start();
    EXPECT_TRUE(adapter.isRunning());

    // Second start call is idempotent
    adapter.start();
    EXPECT_TRUE(adapter.isRunning());

    adapter.stop();
    EXPECT_FALSE(adapter.isRunning());

    // Second stop call is idempotent
    adapter.stop();
    EXPECT_FALSE(adapter.isRunning());
}
