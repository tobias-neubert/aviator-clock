#pragma once

#include "domain/SystemGateway.h"

#if defined(ARDUINO) || defined(ESP32)
#include <Arduino.h>
#include <Preferences.h>
#endif

namespace aviator::platform::system {

/**
 * @brief Platform adapter implementing domain::SystemGateway for the ESP32 platform.
 */
class EspSystemGateway : public domain::SystemGateway {
public:
    void restart() override {
#if defined(ARDUINO) || defined(ESP32)
        ESP.restart();
#endif
    }

    void resetWifiCredentials() override {
#if defined(ARDUINO) || defined(ESP32)
        Preferences preferences;
        if (preferences.begin("wifi-config", false)) {
            preferences.clear();
            preferences.end();
        }
#endif
    }
};

} // namespace aviator::platform::system
