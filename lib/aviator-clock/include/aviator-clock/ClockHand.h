#pragma once

#include <memory>
#include <atomic>

#include <soc/api/ISoc.h>
#include <soc/api/ITask.h>
#include <soc/api/ITime.h>
#include <stepper/api/IStepperMotor.h>

namespace aviator_clock
{
  class ClockHand
  {
  public:
    enum class HandType
    {
      HOUR,
      MINUTE,
      SECOND
    };

    class Builder
    {
    public:
      Builder(soc::api::ISoc &systemOnChip, std::unique_ptr<IStepperMotor> &&stepperMotor);

      Builder &withHandType(HandType type);
      Builder &withTotalAngleForDial(double angle);
      Builder &withMotorSpeedDegreesPerSecond(double speed);
      Builder &withMotorAccelerationDegreesPerSecondSquared(double accel);

      std::unique_ptr<ClockHand> build();

    private:
      soc::api::ISoc &_systemOnChip;
      std::unique_ptr<IStepperMotor> _stepperMotor;
      HandType _type = HandType::SECOND;
      double _totalAngleForDial = 360.0;
      double _motorSpeedDegreesPerSecond = 45.0;
      double _motorAccelerationDegreesPerSecondSquared = 45.0;
    };

    /**
     * @deprecated Use Builder instead.
     */
    ClockHand(HandType type, soc::api::ISoc &systemOnChip,
              std::unique_ptr<IStepperMotor> &&stepperMotor,
              double totalAngleForDial,
              double motorSpeedDegreesPerSecond, double motorAccelerationDegreesPerSecondSquared);

    ~ClockHand();

    void setup();
    void teardown();
    void start();
    
    /**
     * @brief Resumes the task loop without performing a homing sequence.
     * Assumes the hand is already calibrated/homed.
     */
    void resume();
    
    /**
     * @brief Stops the internal task loop.
     */
    void stop();

    /**
     * @brief Moves the hand to a specific display value (e.g. 0-60 for minutes/seconds).
     * Only valid if the hand is homed and not running its internal task loop.
     * @param value The value to display.
     */
    void moveToValue(int value);

  private:
    /**
     * @brief The main task loop with simplified, blocking-style logic.
     */
    void taskLoop();
    void moveToDisplayValue(int value); // Internal helper
    bool runHomingSequence();
    bool setInitialPosition();
    void runTickingLoop();

    HandType _type;
    soc::api::ISoc &_systemOnChip;
    soc::api::Logger &_logger;
    
    soc::api::ITime &_timeProvider;
    std::unique_ptr<IStepperMotor> _stepperMotor;
    std::unique_ptr<soc::api::ITask> _task;
    std::atomic<bool> _shouldStop;

    double _totalAngleForDial;
    double _degreesPerUnit;
    int _lastDisplayValue;
    double _motorSpeedDegreesPerSecond;
    double _motorAccelerationDegreesPerSecondSquared;
    // Flag to indicate if the next task start should skip homing (set via resume())
    bool _skipHomingOnNextStart{false};

    static constexpr uint32_t TASK_STACK_SIZE_BYTES = 4096 * 2;
  };
} // namespace aviator_clock