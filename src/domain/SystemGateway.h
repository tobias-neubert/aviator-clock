#pragma once

namespace aviator::domain {

/**
 * @brief Abstract outbound port for system-level lifecycle and recovery commands.
 */
class SystemGateway {
public:
    virtual ~SystemGateway() = default;

    /**
     * @brief Restarts the micro-controller system.
     */
    virtual void restart() = 0;

    /**
     * @brief Resets and clears stored Wi-Fi credentials in persistent non-volatile storage.
     */
    virtual void resetWifiCredentials() = 0;
};

} // namespace aviator::domain
