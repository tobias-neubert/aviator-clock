#include "platform/web/WebPortalServer.h"

#if defined(ARDUINO) || defined(ESP32)
#include <Arduino.h>
#include <ArduinoJson.h>
#include <array>

namespace {
constexpr const char* PREFERENCES_NAMESPACE_WIFI = "wifi-config";
constexpr const char* PREFERENCES_KEY_SSID = "ssid";
constexpr const char* PREFERENCES_KEY_PASSWORD = "pass";
constexpr size_t MAX_SCANNED_NETWORKS = 16;
} // namespace

#endif

namespace aviator::platform::web {

WebPortalServer::WebPortalServer(
    domain::ClockConfigurationApi& clockConfigurationApi,
    domain::SystemGateway& systemGateway,
    const WebPortalConfiguration& configuration)
    : _clockConfigurationApi(clockConfigurationApi),
      _systemGateway(systemGateway),
      _configuration(configuration)
#if defined(ARDUINO) || defined(ESP32)
      , _server(configuration.port)
#endif
{
}

WebPortalServer::~WebPortalServer() {
    stop();
}

void WebPortalServer::initialize() {
    if (_isInitialized) {
        return;
    }

#if defined(ARDUINO) || defined(ESP32)
    // Mount LittleFS filesystem once for serving frontend assets
    if (!LittleFS.begin(false)) {
        LittleFS.begin(true);
    }

    // Register all static routes and API endpoints once during initialization
    setupRoutes();
#endif

    _isInitialized = true;
}

void WebPortalServer::start() {
    if (_isRunning) {
        return;
    }

    if (!_isInitialized) {
        initialize();
    }

#if defined(ARDUINO) || defined(ESP32)
    // Configure Access Point and start listening without heap allocations
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(_configuration.accessPointSsid, _configuration.accessPointPassword);
    _server.begin();
#endif

    _isRunning = true;
}

void WebPortalServer::stop() {
    if (!_isRunning) {
        return;
    }

#if defined(ARDUINO) || defined(ESP32)
    // Terminate listening and disconnect SoftAP; routes remain registered
    _server.end();
    WiFi.softAPdisconnect(true);
#endif

    _isRunning = false;
}

bool WebPortalServer::isRunning() const {
    return _isRunning;
}

void WebPortalServer::update() {
#if defined(ARDUINO) || defined(ESP32)
    if (_pendingReboot.load(std::memory_order_acquire)) {
        if (static_cast<int32_t>(millis() - _rebootTargetTimeMs) >= 0) {
            _pendingReboot.store(false, std::memory_order_release);
            _systemGateway.restart();
        }
    }

    if (_pendingStop.load(std::memory_order_acquire)) {
        if (static_cast<int32_t>(millis() - _stopTargetTimeMs) >= 0) {
            _pendingStop.store(false, std::memory_order_release);
            stop();
        }
    }
#endif
}

#if defined(ARDUINO) || defined(ESP32)
void WebPortalServer::setupRoutes() {
    // 1. Wi-Fi Scan endpoint (Zero std::map, fixed array deduplication, streaming JSON)
    _server.on("/api/wifi/scan", HTTP_GET, [](AsyncWebServerRequest* request) {
        const int numberOfNetworks = WiFi.scanNetworks();

        struct ScannedNetworkEntry {
            uint8_t index;
            int32_t rssi;
        };
        std::array<ScannedNetworkEntry, MAX_SCANNED_NETWORKS> uniqueNetworks{};
        size_t uniqueCount = 0;

        for (int i = 0; i < numberOfNetworks; ++i) {
            const String currentSsid = WiFi.SSID(i);
            if (currentSsid.length() == 0) {
                continue;
            }
            const int32_t currentRssi = WiFi.RSSI(i);

            int existingSlot = -1;
            for (size_t j = 0; j < uniqueCount; ++j) {
                if (currentSsid.equals(WiFi.SSID(uniqueNetworks[j].index))) {
                    existingSlot = static_cast<int>(j);
                    break;
                }
            }

            if (existingSlot >= 0) {
                if (currentRssi > uniqueNetworks[existingSlot].rssi) {
                    uniqueNetworks[existingSlot].index = static_cast<uint8_t>(i);
                    uniqueNetworks[existingSlot].rssi = currentRssi;
                }
            } else if (uniqueCount < MAX_SCANNED_NETWORKS) {
                uniqueNetworks[uniqueCount++] = {static_cast<uint8_t>(i), currentRssi};
            }
        }

        JsonDocument document;
        JsonArray networkList = document.to<JsonArray>();

        for (size_t i = 0; i < uniqueCount; ++i) {
            JsonObject networkItem = networkList.add<JsonObject>();
            networkItem["ssid"] = WiFi.SSID(uniqueNetworks[i].index);
            networkItem["rssi"] = uniqueNetworks[i].rssi;
            networkItem["protected"] = (WiFi.encryptionType(uniqueNetworks[i].index) != WIFI_AUTH_OPEN);
        }

        AsyncResponseStream* response = request->beginResponseStream("application/json");
        serializeJson(document, *response);
        request->send(response);
    });

    // 2. Wi-Fi Connect endpoint
    _server.on(
        "/api/wifi/connect",
        HTTP_POST,
        [](AsyncWebServerRequest* request) {
            // Completed in onBody handler
        },
        nullptr,
        [](AsyncWebServerRequest* request, uint8_t* data, size_t length, size_t index, size_t total) {
            JsonDocument document;
            const DeserializationError error = deserializeJson(document, data, length);
            if (error) {
                request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
                return;
            }

            const char* ssid = document["ssid"];
            const char* password = document["password"] | "";

            if (ssid == nullptr || strlen(ssid) == 0) {
                request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"SSID is required\"}");
                return;
            }

            Preferences preferences;
            if (preferences.begin(PREFERENCES_NAMESPACE_WIFI, false)) {
                preferences.putString(PREFERENCES_KEY_SSID, ssid);
                preferences.putString(PREFERENCES_KEY_PASSWORD, password);
                preferences.end();
                request->send(200, "application/json", "{\"status\":\"ok\"}");
            } else {
                request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Failed to persist credentials\"}");
            }
        });

    // 3. System Status endpoint (streaming JSON, no intermediate String allocation)
    _server.on("/api/system/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        const domain::SystemStatus systemStatus = _clockConfigurationApi.getSystemStatus();
        JsonDocument document;

        char ipAddressBuffer[16]{};
        if (WiFi.getMode() & WIFI_AP) {
            WiFi.softAPIP().toString().toCharArray(ipAddressBuffer, sizeof(ipAddressBuffer));
        } else {
            WiFi.localIP().toString().toCharArray(ipAddressBuffer, sizeof(ipAddressBuffer));
        }

        document["ip"] = ipAddressBuffer;
        document["connected"] = (WiFi.status() == WL_CONNECTED);
        document["isWifiConnected"] = systemStatus.isWifiConnected;
        document["isTimeSynchronized"] = systemStatus.isTimeSynchronized;
        document["isAlarmRinging"] = systemStatus.isAlarmRinging;

        switch (systemStatus.operatingMode) {
            case domain::OperatingMode::Initial:
                document["operatingMode"] = "Initial";
                break;
            case domain::OperatingMode::NormalRunning:
                document["operatingMode"] = "NormalRunning";
                break;
            case domain::OperatingMode::SetupMode:
                document["operatingMode"] = "SetupMode";
                break;
        }

        AsyncResponseStream* response = request->beginResponseStream("application/json");
        serializeJson(document, *response);
        request->send(response);
    });

    // 4. System Reboot endpoint (Zero xTaskCreate heap allocation)
    _server.on("/api/system/reboot", HTTP_POST, [this](AsyncWebServerRequest* request) {
        request->send(200, "application/json", "{\"status\":\"rebooting\"}");
        _rebootTargetTimeMs = millis() + 500;
        _pendingReboot.store(true, std::memory_order_release);
    });

    // 5. Setup Finish endpoint (Zero xTaskCreate heap allocation)
    _server.on("/api/setup/finish", HTTP_POST, [this](AsyncWebServerRequest* request) {
        request->send(200, "application/json", "{\"status\":\"finishing\"}");
        _stopTargetTimeMs = millis() + 1000;
        _pendingStop.store(true, std::memory_order_release);
    });

    // 6. Alarm Query endpoint (streaming JSON)
    _server.on("/api/alarm", HTTP_GET, [this](AsyncWebServerRequest* request) {
        const domain::Alarm alarm = _clockConfigurationApi.getAlarm();
        JsonDocument document;
        document["hour"] = alarm.time.hours;
        document["minute"] = alarm.time.minutes;
        document["soundTrackId"] = alarm.soundTrackId;
        document["enabled"] = alarm.enabled;

        AsyncResponseStream* response = request->beginResponseStream("application/json");
        serializeJson(document, *response);
        request->send(response);
    });

    // 7. Alarm Update endpoint
    _server.on(
        "/api/alarm",
        HTTP_POST,
        [](AsyncWebServerRequest* request) {
            // Completed in onBody handler
        },
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t length, size_t index, size_t total) {
            JsonDocument document;
            const DeserializationError error = deserializeJson(document, data, length);
            if (error) {
                request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
                return;
            }

            uint8_t hour = 0;
            if (document["hour"].is<uint8_t>()) {
                hour = document["hour"].as<uint8_t>();
            } else if (document["hours"].is<uint8_t>()) {
                hour = document["hours"].as<uint8_t>();
            }

            uint8_t minute = 0;
            if (document["minute"].is<uint8_t>()) {
                minute = document["minute"].as<uint8_t>();
            } else if (document["minutes"].is<uint8_t>()) {
                minute = document["minutes"].as<uint8_t>();
            }

            const uint16_t soundTrackId = document["soundTrackId"] | 1;
            const bool enabled = document["enabled"] | false;

            if (hour >= 24 || minute >= 60 || soundTrackId == 0) {
                request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid alarm parameters\"}");
                return;
            }

            const domain::Alarm alarm{domain::Time{hour, minute, 0}, soundTrackId, enabled};
            const bool success = _clockConfigurationApi.updateAlarm(alarm);
            if (success) {
                request->send(200, "application/json", "{\"status\":\"ok\"}");
            } else {
                request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Failed to persist alarm\"}");
            }
        });

    // 8. Audio Tracks list endpoint (static constant JSON, zero allocation)
    _server.on("/api/audio/tracks", HTTP_GET, [](AsyncWebServerRequest* request) {
        static constexpr const char* AUDIO_TRACKS_JSON =
            "["
            "{\"id\":1,\"name\":\"Alarm Tone 1 (Chime)\"},"
            "{\"id\":2,\"name\":\"Alarm Tone 2 (Classic Bell)\"},"
            "{\"id\":3,\"name\":\"Alarm Tone 3 (Aviation Alert)\"},"
            "{\"id\":4,\"name\":\"Alarm Tone 4 (Gentle Melody)\"},"
            "{\"id\":5,\"name\":\"Alarm Tone 5 (Digital Pulse)\"}"
            "]";
        request->send(200, "application/json", AUDIO_TRACKS_JSON);
    });

    // 9. Audio Preview Test endpoint
    _server.on(
        "/api/audio/test",
        HTTP_POST,
        [](AsyncWebServerRequest* request) {
            // Completed in onBody handler
        },
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t length, size_t index, size_t total) {
            JsonDocument document;
            const DeserializationError error = deserializeJson(document, data, length);
            if (error) {
                request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
                return;
            }

            uint16_t trackId = 0;
            if (document["trackId"].is<uint16_t>()) {
                trackId = document["trackId"].as<uint16_t>();
            } else if (document["id"].is<uint16_t>()) {
                trackId = document["id"].as<uint16_t>();
            } else if (document["track"].is<uint16_t>()) {
                trackId = document["track"].as<uint16_t>();
            }

            const char* action = document["action"] | "";
            if (strcmp(action, "stop") == 0 || trackId == 0) {
                _clockConfigurationApi.stopAudioPreview();
            } else {
                _clockConfigurationApi.previewSoundTrack(trackId);
            }

            request->send(200, "application/json", "{\"status\":\"ok\"}");
        });

    // 10. Static files from LittleFS
    _server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
}
#endif

} // namespace aviator::platform::web
