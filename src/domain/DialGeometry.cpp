#include "domain/DialGeometry.h"

namespace aviator::domain {

DialGeometry::DialGeometry(double activeArcDegrees)
    : _activeArcDegrees(activeArcDegrees > 0.0 && activeArcDegrees <= FULL_CIRCLE_DEGREES
                            ? activeArcDegrees
                            : DEFAULT_ACTIVE_ARC_DEGREES),
      _degreesPerHour(_activeArcDegrees / static_cast<double>(UNITS_PER_HOUR_DIAL)),
      _degreesPerMinute(_activeArcDegrees / static_cast<double>(UNITS_PER_MINUTE_DIAL)),
      _degreesPerSecond(_activeArcDegrees / static_cast<double>(UNITS_PER_SECOND_DIAL)) {}

double DialGeometry::hourToDegrees(uint8_t hours) const {
    const uint8_t displayHour = hours % UNITS_PER_HOUR_DIAL;
    return static_cast<double>(displayHour) * _degreesPerHour;
}

double DialGeometry::minuteToDegrees(uint8_t minutes) const {
    const uint8_t displayMinute = minutes % UNITS_PER_MINUTE_DIAL;
    return static_cast<double>(displayMinute) * _degreesPerMinute;
}

double DialGeometry::secondToDegrees(uint8_t seconds) const {
    const uint8_t displaySecond = seconds % UNITS_PER_SECOND_DIAL;
    return static_cast<double>(displaySecond) * _degreesPerSecond;
}

bool DialGeometry::isHourWrapAround(uint8_t previousHours, uint8_t currentHours) const {
    const uint8_t previousDisplayHour = previousHours % UNITS_PER_HOUR_DIAL;
    const uint8_t currentDisplayHour = currentHours % UNITS_PER_HOUR_DIAL;
    return (previousDisplayHour == (UNITS_PER_HOUR_DIAL - 1)) && (currentDisplayHour == 0);
}

bool DialGeometry::isMinuteWrapAround(uint8_t previousMinutes, uint8_t currentMinutes) const {
    const uint8_t previousDisplayMinute = previousMinutes % UNITS_PER_MINUTE_DIAL;
    const uint8_t currentDisplayMinute = currentMinutes % UNITS_PER_MINUTE_DIAL;
    return (previousDisplayMinute == (UNITS_PER_MINUTE_DIAL - 1)) && (currentDisplayMinute == 0);
}

bool DialGeometry::isSecondWrapAround(uint8_t previousSeconds, uint8_t currentSeconds) const {
    const uint8_t previousDisplaySecond = previousSeconds % UNITS_PER_SECOND_DIAL;
    const uint8_t currentDisplaySecond = currentSeconds % UNITS_PER_SECOND_DIAL;
    return (previousDisplaySecond == (UNITS_PER_SECOND_DIAL - 1)) && (currentDisplaySecond == 0);
}

double DialGeometry::calculateHourTargetAngle(uint8_t currentHours, uint8_t previousHours) const {
    if (isHourWrapAround(previousHours, currentHours)) {
        return FULL_CIRCLE_DEGREES;
    }
    return hourToDegrees(currentHours);
}

double DialGeometry::calculateMinuteTargetAngle(uint8_t currentMinutes, uint8_t previousMinutes) const {
    if (isMinuteWrapAround(previousMinutes, currentMinutes)) {
        return FULL_CIRCLE_DEGREES;
    }
    return minuteToDegrees(currentMinutes);
}

double DialGeometry::calculateSecondTargetAngle(uint8_t currentSeconds, uint8_t previousSeconds) const {
    if (isSecondWrapAround(previousSeconds, currentSeconds)) {
        return FULL_CIRCLE_DEGREES;
    }
    return secondToDegrees(currentSeconds);
}

double DialGeometry::alarmPeriodIndicatorToDegrees(bool isPostMeridiem) const {
    // 0° represents AM, 165° represents PM on the seconds dial
    return isPostMeridiem ? (_activeArcDegrees / 2.0) : 0.0;
}

double DialGeometry::getActiveArcDegrees() const {
    return _activeArcDegrees;
}

double DialGeometry::getDegreesPerHour() const {
    return _degreesPerHour;
}

double DialGeometry::getDegreesPerMinute() const {
    return _degreesPerMinute;
}

double DialGeometry::getDegreesPerSecond() const {
    return _degreesPerSecond;
}

} // namespace aviator::domain
