#pragma once

namespace aviator::platform::sensor {

/**
 * @brief Abstract interface for binary digital input reading (e.g. Hall sensors, limit switches).
 */
class DigitalInput {
public:
    virtual ~DigitalInput() = default;

    /**
     * @brief Checks if the digital input is currently in its active state.
     * @return True if active (e.g. magnet detected / switch engaged), false otherwise.
     */
    virtual bool isActive() const = 0;
};

} // namespace aviator::platform::sensor
