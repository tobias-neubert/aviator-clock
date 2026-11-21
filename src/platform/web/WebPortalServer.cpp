#include "platform/web/WebPortalServer.h"

#if defined(ARDUINO) || defined(ESP32)
#include <Arduino.h>
#include <ArduinoJson.h>
#include <map>

namespace {
constexpr const char* PREFERENCES_NAMESPACE_WIFI = "wifi-config";
constexpr const char* PREFERENCES_KEY_SSID = "ssid";
constexpr const char* PREFERENCES_KEY_PASSWORD = "pass";
} // namespace

#endif

namespace aviator::platform::web {

WebPortalServer::WebPortalServer(
    domain::ClockConfigurationApi& clockConfigurationApi,
    domain::SystemGateway& systemGateway,
    const WebPortalConfiguration& configuration)
    : _clockConfigurationApi(clockConfigurationApi),
      _systemGateway(systemGateway),
      _configuration(configuration) {
}

WebPortalServer::~WebPortalServer() {
    stop();
}

void WebPortalServer::start() {
    if (_isRunning) {
        return;
    }

#if defined(ARDUINO) || defined(ESP32)
    // Mount LittleFS filesystem for serving frontend assets
    if (!LittleFS.begin(false)) {
        LittleFS.begin(true);
    }

    // Configure Access Point
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(_configuration.accessPointSsid, _configuration.accessPointPassword);

    _server = std::make_unique<AsyncWebServer>(_configuration.port);
    setupRoutes();
    _server->begin();
#endif

    _isRunning = true;
}

void WebPortalServer::stop() {
    if (!_isRunning) {
        return;
    }

#if defined(ARDUINO) || defined(ESP32)
    if (_server) {
        _server->end();
        _server.reset();
    }
    WiFi.softAPdisconnect(true);
#endif

    _isRunning = false;
}

bool WebPortalServer::isRunning() const {
    return _isRunning;
}

#if defined(ARDUINO) || defined(ESP32)
void WebPortalServer::setupRoutes() {
    // 1. Wi-Fi Scan endpoint
    _server->on("/api/wifi/scan", HTTP_GET, [](AsyncWebServerRequest* request) {
        const int numberOfNetworks = WiFi.scanNetworks();
        JsonDocument document;
        JsonArray networkList = document.to<JsonArray>();

        std::map<String, int> bestNetworks;
        for (int i = 0; i < numberOfNetworks; ++i) {
            String ssid = WiFi.SSID(i);
            if (ssid.length() == 0) {
                continue;
            }
            if (bestNetworks.find(ssid) == bestNetworks.end()) {
                bestNetworks[ssid] = i;
            } else if (WiFi.RSSI(i) > WiFi.RSSI(bestNetworks[ssid])) {
                bestNetworks[ssid] = i;
            }
        }

        for (const auto& [ssid, index] : bestNetworks) {
            JsonObject networkItem = networkList.add<JsonObject>();
            networkItem["ssid"] = WiFi.SSID(index);
            networkItem["rssi"] = WiFi.RSSI(index);
            networkItem["protected"] = (WiFi.encryptionType(index) != WIFI_AUTH_OPEN);
        }

        String response;
        serializeJson(document, response);
        request->send(200, "application/json", response);
    });

    // 2. Wi-Fi Connect endpoint
    _server->on(
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

    // 3. System Status endpoint
    _server->on("/api/system/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        const domain::SystemStatus systemStatus = _clockConfigurationApi.getSystemStatus();
        JsonDocument document;

        const String ipAddress = (WiFi.getMode() & WIFI_AP) ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
        document["ip"] = ipAddress;
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

        String response;
        serializeJson(document, response);
        request->send(200, "application/json", response);
    });

    // 4. System Reboot endpoint
    _server->on("/api/system/reboot", HTTP_POST, [this](AsyncWebServerRequest* request) {
        request->send(200, "application/json", "{\"status\":\"rebooting\"}");

        xTaskCreate([](void* context) {
            auto* gateway = static_cast<domain::SystemGateway*>(context);
            vTaskDelay(pdMS_TO_TICKS(500));
            gateway->restart();
            vTaskDelete(NULL);
        }, "deferred_restart_task", 2048, &_systemGateway, 1, NULL);
    });

    // 5. Setup Finish endpoint
    _server->on("/api/setup/finish", HTTP_POST, [this](AsyncWebServerRequest* request) {
        request->send(200, "application/json", "{\"status\":\"finishing\"}");

        xTaskCreate([](void* context) {
            auto* server = static_cast<WebPortalServer*>(context);
            vTaskDelay(pdMS_TO_TICKS(1000));
            server->stop();
            vTaskDelete(NULL);
        }, "deferred_stop_task", 4096, this, 1, NULL);
    });

    // 6. Alarm Query endpoint
    _server->on("/api/alarm", HTTP_GET, [this](AsyncWebServerRequest* request) {
        const domain::Alarm alarm = _clockConfigurationApi.getAlarm();
        JsonDocument document;
        document["hour"] = alarm.time.hours;
        document["minute"] = alarm.time.minutes;
        document["soundTrackId"] = alarm.soundTrackId;
        document["enabled"] = alarm.enabled;

        String response;
        serializeJson(document, response);
        request->send(200, "application/json", response);
    });

    // 7. Alarm Update endpoint
    _server->on(
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

    // 8. Audio Tracks list endpoint
    _server->on("/api/audio/tracks", HTTP_GET, [](AsyncWebServerRequest* request) {
        JsonDocument document;
        JsonArray tracksArray = document.to<JsonArray>();

        const struct { uint16_t id; const char* name; } availableTracks[] = {
            {1, "Alarm Tone 1 (Chime)"},
            {2, "Alarm Tone 2 (Classic Bell)"},
            {3, "Alarm Tone 3 (Aviation Alert)"},
            {4, "Alarm Tone 4 (Gentle Melody)"},
            {5, "Alarm Tone 5 (Digital Pulse)"}
        };

        for (const auto& track : availableTracks) {
            JsonObject item = tracksArray.add<JsonObject>();
            item["id"] = track.id;
            item["name"] = track.name;
        }

        String response;
        serializeJson(document, response);
        request->send(200, "application/json", response);
    });

    // 9. Audio Preview Test endpoint
    _server->on(
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
    _server->serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
}
#endif

} // namespace aviator::platform::web
