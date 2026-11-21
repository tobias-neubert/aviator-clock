#pragma once

#include <cstdint>

namespace aviator::domain {

/**
 * @brief DialGeometry encapsulates the mathematical transformations between time units
 * and physical angles on the 330.0° active arc analog dials, including deterministic
 * forward clockwise wrap-around jumps across the 30.0° inactive gauge sector.
 */
class DialGeometry {
public:
    static constexpr double DEFAULT_ACTIVE_ARC_DEGREES = 330.0;
    static constexpr double FULL_CIRCLE_DEGREES = 360.0;
    static constexpr uint8_t UNITS_PER_HOUR_DIAL = 12;
    static constexpr uint8_t UNITS_PER_MINUTE_DIAL = 60;
    static constexpr uint8_t UNITS_PER_SECOND_DIAL = 60;

    explicit DialGeometry(double activeArcDegrees = DEFAULT_ACTIVE_ARC_DEGREES);

    double hourToDegrees(uint8_t hours) const;
    double minuteToDegrees(uint8_t minutes) const;
    double secondToDegrees(uint8_t seconds) const;

    bool isHourWrapAround(uint8_t previousHours, uint8_t currentHours) const;
    bool isMinuteWrapAround(uint8_t previousMinutes, uint8_t currentMinutes) const;
    bool isSecondWrapAround(uint8_t previousSeconds, uint8_t currentSeconds) const;

    double calculateHourTargetAngle(uint8_t currentHours, uint8_t previousHours) const;
    double calculateMinuteTargetAngle(uint8_t currentMinutes, uint8_t previousMinutes) const;
    double calculateSecondTargetAngle(uint8_t currentSeconds, uint8_t previousSeconds) const;

    double alarmPeriodIndicatorToDegrees(bool isPostMeridiem) const;

    double getActiveArcDegrees() const;
    double getDegreesPerHour() const;
    double getDegreesPerMinute() const;
    double getDegreesPerSecond() const;

private:
    double _activeArcDegrees;
    double _degreesPerHour;
    double _degreesPerMinute;
    double _degreesPerSecond;
};

} // namespace aviator::domain
