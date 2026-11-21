#pragma once

#include "domain/DialGeometry.h"
#include "domain/HandActuator.h"
#include "domain/Time.h"

namespace aviator::domain {

/**
 * @brief ClockEngine coordinates the three dial hand actuators (hours, minutes, seconds)
 * using DialGeometry to position them accurately and handle wrap-around jumps.
 */
class ClockEngine {
public:
    ClockEngine(HandActuator& hourHand,
                HandActuator& minuteHand,
                HandActuator& secondHand,
                const DialGeometry& dialGeometry = DialGeometry());

    void updateTime(const Time& currentTime);

    bool homeAll();
    bool isAllHomed() const;
    bool isBusy() const;
    void stopAll();

    const DialGeometry& getDialGeometry() const;
    const Time& getLastTime() const;

private:
    HandActuator& _hourHand;
    HandActuator& _minuteHand;
    HandActuator& _secondHand;
    DialGeometry _dialGeometry;

    Time _lastTime{};
    bool _hasInitialTime{false};
};

} // namespace aviator::domain
