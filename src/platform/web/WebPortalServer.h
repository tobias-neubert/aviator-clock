#pragma once

#include <cstdint>
#include <memory>
#include "domain/ClockConfigurationApi.h"
#include "domain/SystemGateway.h"

#if defined(ARDUINO) || defined(ESP32)
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <WiFi.h>
#endif

namespace aviator::platform::web {

/**
 * @brief Configuration parameters for the web portal server.
 */
struct WebPortalConfiguration {
    uint16_t port{80};
    const char* accessPointSsid{"Aviator-Clock-Setup"};
    const char* accessPointPassword{nullptr};
};

using WebPortalConfig = WebPortalConfiguration;

/**
 * @brief Platform adapter hosting ESPAsyncWebServer and serving LittleFS assets and REST APIs.
 * 
 * Provides HTTP REST endpoints for Wi-Fi scanning/provisioning, alarm configuration,
 * audio track previewing, and system lifecycle management. Translates inbound REST operations
 * directly into domain port calls on ClockConfigurationApi and SystemGateway.
 */
class WebPortalServer {
public:
    WebPortalServer(
        domain::ClockConfigurationApi& clockConfigurationApi,
        domain::SystemGateway& systemGateway,
        const WebPortalConfiguration& configuration = WebPortalConfiguration{});

    ~WebPortalServer();

    /**
     * @brief Mounts LittleFS, starts Wi-Fi SoftAP, registers routes, and begins listening.
     */
    void start();

    /**
     * @brief Stops listening and terminates SoftAP mode.
     */
    void stop();

    bool isRunning() const;

private:
#if defined(ARDUINO) || defined(ESP32)
    void setupRoutes();
#endif

    domain::ClockConfigurationApi& _clockConfigurationApi;
    domain::SystemGateway& _systemGateway;
    WebPortalConfiguration _configuration;
    bool _isRunning{false};

#if defined(ARDUINO) || defined(ESP32)
    std::unique_ptr<AsyncWebServer> _server;
#endif
};

} // namespace aviator::platform::web
