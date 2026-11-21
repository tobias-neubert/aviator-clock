#pragma once

#include <cstdint>

namespace aviator::platform::board_support_package {

/**
 * @brief Pin mapping for a single dial stepper motor and its Hall effect zero sensor.
 */
struct StepperPinConfig {
    uint8_t stepPin;
    uint8_t directionPin;
    uint8_t hallSensorPin;
};

/**
 * @brief Complete pinout configuration for the Aviator Clock hardware platform.
 * ESP32 NodeMCU-32S default pin assignments:
 * - Second Hand: Step 14, Dir 12, Hall 25
 * - Minute Hand: Step 27, Dir 26, Hall 34
 * - Hour Hand:   Step 19, Dir 18, Hall 35
 * - Common Motor Enable: Pin 33 (Active LOW)
 * - Single Setup Button: Pin 15 (Active LOW with pullup)
 */
struct HardwarePinConfig {
    StepperPinConfig hourHand{19, 18, 35};
    StepperPinConfig minuteHand{27, 26, 34};
    StepperPinConfig secondHand{14, 12, 25};
    uint8_t motorEnablePin{33};
    uint8_t setupButtonPin{15};
};

} // namespace aviator::platform::board_support_package
