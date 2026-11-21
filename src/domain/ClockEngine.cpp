#include "domain/ClockEngine.h"

namespace aviator::domain {

ClockEngine::ClockEngine(HandActuator& hourHand,
                         HandActuator& minuteHand,
                         HandActuator& secondHand,
                         const DialGeometry& dialGeometry)
    : _hourHand(hourHand),
      _minuteHand(minuteHand),
      _secondHand(secondHand),
      _dialGeometry(dialGeometry) {}

void ClockEngine::updateTime(const Time& currentTime) {
    if (!_hasInitialTime) {
        _hourHand.setTargetAngle(_dialGeometry.hourToDegrees(currentTime.hours));
        _minuteHand.setTargetAngle(_dialGeometry.minuteToDegrees(currentTime.minutes));
        _secondHand.setTargetAngle(_dialGeometry.secondToDegrees(currentTime.seconds));
        _lastTime = currentTime;
        _hasInitialTime = true;
        return;
    }

    if (_hourHand.getCurrentAngle() >= DialGeometry::FULL_CIRCLE_DEGREES) {
        _hourHand.setCurrentAngle(0.0);
    }
    if (_minuteHand.getCurrentAngle() >= DialGeometry::FULL_CIRCLE_DEGREES) {
        _minuteHand.setCurrentAngle(0.0);
    }
    if (_secondHand.getCurrentAngle() >= DialGeometry::FULL_CIRCLE_DEGREES) {
        _secondHand.setCurrentAngle(0.0);
    }

    const double hourTarget = _dialGeometry.calculateHourTargetAngle(currentTime.hours, _lastTime.hours);
    const double minuteTarget = _dialGeometry.calculateMinuteTargetAngle(currentTime.minutes, _lastTime.minutes);
    const double secondTarget = _dialGeometry.calculateSecondTargetAngle(currentTime.seconds, _lastTime.seconds);

    _hourHand.setTargetAngle(hourTarget);
    _minuteHand.setTargetAngle(minuteTarget);
    _secondHand.setTargetAngle(secondTarget);

    _lastTime = currentTime;
}

bool ClockEngine::homeAll() {
    const bool hourResult = _hourHand.home();
    const bool minuteResult = _minuteHand.home();
    const bool secondResult = _secondHand.home();
    return hourResult && minuteResult && secondResult;
}

bool ClockEngine::isAllHomed() const {
    return _hourHand.isHomed() && _minuteHand.isHomed() && _secondHand.isHomed();
}

bool ClockEngine::isBusy() const {
    return _hourHand.isBusy() || _minuteHand.isBusy() || _secondHand.isBusy();
}

void ClockEngine::stopAll() {
    _hourHand.stop();
    _minuteHand.stop();
    _secondHand.stop();
}

const DialGeometry& ClockEngine::getDialGeometry() const {
    return _dialGeometry;
}

const Time& ClockEngine::getLastTime() const {
    return _lastTime;
}

} // namespace aviator::domain
