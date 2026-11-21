#pragma once

#include "domain/HandActuator.h"
#include "platform/board_support_package/PinConfig.h"
#include "platform/motor/AccelStepperWrapper.h"
#include "platform/motor/HomingStrategy.h"
#include "platform/motor/PreciseOneWayHomingStrategy.h"
#include "platform/motor/StepperHandActuator.h"
#include <soc/api/ISoc.h>
#include <soc/api/ITask.h>
#include <atomic>
#include <memory>

namespace aviator::platform::motor {

/**
 * @brief Complete hardware configuration for the 3-axis StepperSubsystem.
 */
struct StepperSubsystemHardwareConfig {
    board_support_package::StepperPinConfig hourPins{19, 18, 35};
    board_support_package::StepperPinConfig minutePins{27, 26, 34};
    board_support_package::StepperPinConfig secondPins{14, 12, 25};
    uint8_t motorEnablePin{33};
    StepperActuatorConfig actuatorConfig{};
    PreciseHomingConfig homingConfig{};
};

/**
 * @brief Centralized Stepper Motor Execution Subsystem.
 * 
 * Per ADR 0003, replaces per-hand FreeRTOS tasks with a single, dedicated high-priority
 * motor runner task that steps all three dials (Hours, Minutes, Seconds) simultaneously
 * without blocking delays inside the pulse generation loop.
 */
class StepperSubsystem {
public:
    using HardwareConfig = StepperSubsystemHardwareConfig;

    /**
     * @brief Dependency injection constructor for unit testing and custom compositions.
     */
    StepperSubsystem(
        soc::api::ISoc& systemOnChip,
        StepperHandActuator& hourHand,
        StepperHandActuator& minuteHand,
        StepperHandActuator& secondHand);

#if defined(AVIATOR_ACCEL_STEPPER_AVAILABLE)
    /**
     * @brief Target hardware constructor initializing drivers and Hall sensors from pin mappings.
     */
    explicit StepperSubsystem(
        soc::api::ISoc& systemOnChip,
        const StepperSubsystemHardwareConfig& config = StepperSubsystemHardwareConfig{});
#endif

    ~StepperSubsystem();

    /**
     * @brief Spawns the centralized motor runner task once during initialization.
     */
    void start();

    /**
     * @brief Stops the motor runner task and halts all actuators.
     */
    void stop();

    bool isRunning() const;

    /**
     * @brief Executes one non-blocking stepping iteration across all three dial actuators.
     * Exported publicly for deterministic host-based unit testing.
     * @return True if any motor is actively moving or homing, false if all three are idle.
     */
    bool stepIteration();

    // Outbound HandActuator domain port accessors
    aviator::domain::HandActuator& getHourHand();
    aviator::domain::HandActuator& getMinuteHand();
    aviator::domain::HandActuator& getSecondHand();

    // Concrete actuator accessors
    StepperHandActuator& getHourHandActuator();
    StepperHandActuator& getMinuteHandActuator();
    StepperHandActuator& getSecondHandActuator();

    // Subsystem-wide dial coordination
    bool homeAll();
    bool isAllHomed() const;
    bool isBusy() const;
    void stopAll();

private:
    void runMotorRunnerLoop();

    soc::api::ISoc& _systemOnChip;
    soc::api::Logger& _logger;

#if defined(AVIATOR_ACCEL_STEPPER_AVAILABLE)
    // Hardware-owned instances when constructed directly from pin config
    std::unique_ptr<StepperController> _hourController;
    std::unique_ptr<StepperController> _minuteController;
    std::unique_ptr<StepperController> _secondController;

    std::unique_ptr<soc::api::IDigitalInput> _hourSensor;
    std::unique_ptr<soc::api::IDigitalInput> _minuteSensor;
    std::unique_ptr<soc::api::IDigitalInput> _secondSensor;

    std::unique_ptr<HomingStrategy> _hourHoming;
    std::unique_ptr<HomingStrategy> _minuteHoming;
    std::unique_ptr<HomingStrategy> _secondHoming;

    std::unique_ptr<StepperHandActuator> _ownedHourActuator;
    std::unique_ptr<StepperHandActuator> _ownedMinuteActuator;
    std::unique_ptr<StepperHandActuator> _ownedSecondActuator;
#endif

    // References to the 3 active dial actuators
    StepperHandActuator& _hourHandActuator;
    StepperHandActuator& _minuteHandActuator;
    StepperHandActuator& _secondHandActuator;

    std::atomic<bool> _isRunning{false};
    std::unique_ptr<soc::api::ITask> _runnerTask;
};

} // namespace aviator::platform::motor
