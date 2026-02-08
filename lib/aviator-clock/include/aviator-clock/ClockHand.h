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

    ClockHand(HandType type, soc::api::ISoc &soc,
              std::unique_ptr<IStepperMotor> &&stepperMotor,
              double totalAngleForDial,
              double motorSpeedDps, double motorAccelerationDps2);

    ~ClockHand();

    void setup();
    void teardown();
    void start();
    
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
    soc::api::ISoc &_soc;
    soc::api::Logger &_logger;
    
    soc::api::ITime &_timeProvider;
    std::unique_ptr<IStepperMotor> _stepperMotor;
    std::unique_ptr<soc::api::ITask> _task;
    std::atomic<bool> _shouldStop;

    double _totalAngleForDial;
    double _degreesPerUnit;
    int _lastDisplayValue;
    double _motorSpeedDps;
    double _motorAccelerationDps2;
  };
} // namespace aviator_clock