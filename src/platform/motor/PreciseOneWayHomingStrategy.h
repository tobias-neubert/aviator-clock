#pragma once

#include "platform/logging/Logger.h"
#include "platform/motor/HomingStrategy.h"
#include "platform/motor/StepperController.h"
#include "platform/sensor/DigitalInput.h"

namespace aviator::platform::motor {

/**
 * @brief Configuration parameters for PreciseOneWayHomingStrategy.
 */
struct PreciseHomingConfig {
    float homingSpeed{200.0f};           // Speed for seeking the sensor in steps/sec
    float homingAcceleration{1000.0f};   // Acceleration for homing in steps/sec^2
    long maxHomingTravel{3200};          // Maximum distance before timeout (2 full revolutions at 1600 steps/rev)
    int moveDirection{1};                // +1 for clockwise, -1 for counter-clockwise
    long backOffDistance{100};           // Steps to back off if starting directly over the sensor
};

/**
 * @brief High-precision, one-way clockwise homing strategy for dial indicators using Hall-effect sensors.
 * 
 * Algorithm:
 * 1. If starting with the sensor already active, back off counter-clockwise by backOffDistance steps.
 *    If the sensor is still active after backing off, report a sensor/motor error.
 * 2. Rotate clockwise seeking the sensor trigger edge up to maxHomingTravel steps.
 * 3. Immediately upon sensor activation (active-low signal), command the motor controller to stop.
 * 4. Once fully decelerated to a standstill, set the current position as 0 (datum point).
 */
class PreciseOneWayHomingStrategy : public HomingStrategy {
public:
    using Config = PreciseHomingConfig;

    PreciseOneWayHomingStrategy(
        StepperController& stepperController,
        sensor::DigitalInput& limitSwitch,
        const PreciseHomingConfig& config = PreciseHomingConfig{},
        logging::Logger* logger = nullptr);

    bool beginHoming() override;
    HomingResult updateHoming() override;
    HomingResult getHomingResult() const override;
    void cancelHoming() override;
    void resetStrategy() override;

    const PreciseHomingConfig& getConfig() const { return _config; }

private:
    enum class Phase {
        NOT_STARTED,
        MOVING_OFF_SWITCH,
        SEEKING_SWITCH,
        DECELERATING,
        DONE
    };

    void enterPhase(Phase newPhase);

    StepperController& _stepperController;
    sensor::DigitalInput& _limitSwitch;
    const PreciseHomingConfig _config;
    logging::Logger* _logger;
    Phase _phase{Phase::NOT_STARTED};
    HomingResult _result{HomingResult::NOT_YET_STARTED};
};

} // namespace aviator::platform::motor
