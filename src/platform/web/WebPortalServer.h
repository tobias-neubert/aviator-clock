#pragma once

#include <atomic>
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
 * 
 * Adheres to zero-runtime-dynamic-allocation and persistent task lifecycle engineering standards.
 */
class WebPortalServer {
public:
    WebPortalServer(
        domain::ClockConfigurationApi& clockConfigurationApi,
        domain::SystemGateway& systemGateway,
        const WebPortalConfiguration& configuration = WebPortalConfiguration{});

    ~WebPortalServer();

    /**
     * @brief Performs one-time setup: mounts LittleFS and registers static routes and API endpoints.
     */
    void initialize();

    /**
     * @brief Starts Wi-Fi SoftAP and begins listening for HTTP requests.
     */
    void start();

    /**
     * @brief Stops listening and terminates SoftAP mode without tearing down handlers.
     */
    void stop();

    bool isRunning() const;

    /**
     * @brief Evaluates non-allocating deferred lifecycle triggers (reboot, finish).
     * Invoked periodically by the system background execution loop.
     */
    void update();

private:
#if defined(ARDUINO) || defined(ESP32)
    void setupRoutes();
#endif

    domain::ClockConfigurationApi& _clockConfigurationApi;
    domain::SystemGateway& _systemGateway;
    WebPortalConfiguration _configuration;
    bool _isInitialized{false};
    bool _isRunning{false};

    // Non-allocating deferred triggers for restart and stop
    std::atomic<bool> _pendingReboot{false};
    uint32_t _rebootTargetTimeMs{0};

    std::atomic<bool> _pendingStop{false};
    uint32_t _stopTargetTimeMs{0};

#if defined(ARDUINO) || defined(ESP32)
    AsyncWebServer _server;
#endif
};

} // namespace aviator::platform::web
