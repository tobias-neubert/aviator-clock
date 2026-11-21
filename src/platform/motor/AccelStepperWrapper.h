#pragma once

#if __has_include(<AccelStepper.h>)
#include <AccelStepper.h>
#include "platform/motor/StepperController.h"

#define AVIATOR_ACCEL_STEPPER_AVAILABLE 1

namespace aviator::platform::motor {

/**
 * @brief Platform adapter wrapping the third-party AccelStepper pulse generation library.
 * 
 * Provides an implementation of the StepperController interface using AccelStepper
 * to output STEP/DIR pulses on target microcontrollers.
 */
class AccelStepperWrapper : public StepperController {
public:
    AccelStepperWrapper(uint8_t stepPin,
                        uint8_t directionPin,
                        AccelStepper::MotorInterfaceType interface = AccelStepper::DRIVER)
        : _accelStepper(interface, stepPin, directionPin) {}

    void setEnablePin(uint8_t enablePin) override {
        _accelStepper.setEnablePin(enablePin);
    }

    void setPinsInverted(bool directionInvert, bool stepInvert, bool enableInvert) override {
        _accelStepper.setPinsInverted(directionInvert, stepInvert, enableInvert);
    }

    void enableOutputs() override {
        _accelStepper.enableOutputs();
    }

    void disableOutputs() override {
        _accelStepper.disableOutputs();
    }

    void setMaxSpeed(float speed) override {
        _accelStepper.setMaxSpeed(speed);
    }

    void setAcceleration(float acceleration) override {
        _accelStepper.setAcceleration(acceleration);
    }

    void moveTo(long absoluteSteps) override {
        _accelStepper.moveTo(absoluteSteps);
    }

    void move(long relativeSteps) override {
        _accelStepper.move(relativeSteps);
    }

    long getCurrentPosition() const override {
        return const_cast<AccelStepper&>(_accelStepper).currentPosition();
    }

    void setCurrentPosition(long absoluteSteps) override {
        _accelStepper.setCurrentPosition(absoluteSteps);
    }

    long distanceToGo() const override {
        return const_cast<AccelStepper&>(_accelStepper).distanceToGo();
    }

    bool run() override {
        return _accelStepper.run();
    }

    void stop() override {
        _accelStepper.stop();
    }

private:
    AccelStepper _accelStepper;
};

} // namespace aviator::platform::motor

#endif // __has_include(<AccelStepper.h>)
