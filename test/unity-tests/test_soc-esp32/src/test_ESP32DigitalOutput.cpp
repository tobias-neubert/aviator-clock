#include <unity.h>
#include <map>
#include <Arduino.h>

// pull in your implementation and API
#include <soc/api/IDigitalOutput.h>
#include <soc/esp32/ESP32DigitalOutput.h>

using soc::api::IDigitalOutput;
using soc::esp32::ESP32DigitalOutput;

// helper to clear state before each test
void setUp(void) {
    fakePinValues().clear();
}

void tearDown(void) {
    fakePinValues().clear();
}

void test_on() {
    const int PIN = 7;
    ESP32DigitalOutput out(PIN);
    out.on();
    TEST_ASSERT_EQUAL_INT(HIGH, fakePinValues()[PIN]);
}

void test_off() {
    const int PIN = 42;
    ESP32DigitalOutput out(PIN);
    out.off();
    TEST_ASSERT_EQUAL_INT(LOW, fakePinValues()[PIN]);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_on);
    RUN_TEST(test_off);
    // …
    return UNITY_END();
}
