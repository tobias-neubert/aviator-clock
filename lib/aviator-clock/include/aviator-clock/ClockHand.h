#pragma once

#include <memory>

#include <soc/api/ISocComponent.h>
#include <soc/api/ITime.h>
#include <soc/api/ILogger.h>
#include <stepper/api/IStepperMotor.h>

namespace aviator_clock
{
    class ClockHand : public soc::api::ISocComponent
    {
    public:
        enum class HandType
        {
            HOUR,
            MINUTE,
            SECOND
        };

        /**
         * @brief Construct a new generic Clock Hand.
         *
         * Dependencies that are "borrowed" (like the logger and time provider)
         * are passed by reference.
         *
         * Dependencies that are "owned" (like the stepper motor) are passed
         * by std::unique_ptr to transfer ownership to this class.
         */
        ClockHand(HandType type,
                  soc::api::ITime &timeProvider,
                  std::unique_ptr<IStepperMotor> stepperMotor,
                  soc::api::ILogger &logger,
                  double totalAngleForDial,
                  double dialStartOffsetDegrees,
                  double motorSpeedDps,
                  double motorAccelerationDps2);

        virtual ~ClockHand() = default;

        void setup() override;
        void advanceState(unsigned long currentTimeMs) override;
        void render() override;
        void teardown() override;

    private:
        HandType _type;
        soc::api::ITime &_timeProvider;
        std::unique_ptr<IStepperMotor> _stepperMotor;
        soc::api::ILogger &_logger;

        double _totalAngleForDial;      // Total angular span of the hand, less than 360 for analog instruments
        double _dialStartOffsetDegrees; // Offset of "0" from motor's physical 0
        double _degreesPerUnit;         // Calculated degrees for each unit tick

        int _lastUnitProcessed;
        bool _isPositionInitialized;
        double _motorSpeedDps;
        double _motorAccelerationDps2;
        bool _homingFailed;
        bool _operationStarted;

        void moveToUnit(int unit);
    };
}