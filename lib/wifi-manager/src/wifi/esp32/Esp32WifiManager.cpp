#include "wifi/esp32/Esp32WifiManager.h"

#ifdef ESP32

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <map>

static const char* TAG = "WifiManager";

namespace wifi
{
    namespace esp32
    {
        Esp32WifiManager::Esp32WifiManager(soc::api::ISoc& soc) : _soc(soc)
        {
        }

        Esp32WifiManager::~Esp32WifiManager()
        {
            stopSetupMode();
        }

        void Esp32WifiManager::begin()
        {
            _preferences.begin("wifi-config", false); 
            String ssid = "";
            String pass = "";

            if (_preferences.isKey("ssid")) {
                ssid = _preferences.getString("ssid", "");
                pass = _preferences.getString("pass", "");
            }
            _preferences.end();

            if (ssid.length() > 0)
            {
                _soc.getLogger().i(TAG, "Connecting to %s...", ssid.c_str());
                WiFi.disconnect();
                WiFi.mode(WIFI_STA);
                delay(100);
                WiFi.begin(ssid.c_str(), pass.c_str());
            }
            else
            {
                _soc.getLogger().i(TAG, "No credentials saved.");
            }
        }

        bool Esp32WifiManager::hasCredentials() const
        {
            Preferences p;
            p.begin("wifi-config", false);
            bool exists = p.isKey("ssid");
            p.end();
            return exists;
        }

        bool Esp32WifiManager::isConnected() const
        {
            return WiFi.status() == WL_CONNECTED;
        }

        void Esp32WifiManager::resetCredentials()
        {
            _soc.getLogger().w(TAG, "Clearing WiFi credentials...");
            _preferences.begin("wifi-config", false);
            _preferences.clear();
            _preferences.end();
            _soc.getLogger().i(TAG, "Credentials cleared.");
        }

        bool Esp32WifiManager::isSetupModeActive() const
        {
            return _isSetupMode;
        }

        void Esp32WifiManager::startSetupMode()
        {
            if (_isSetupMode) return;

            _soc.getLogger().i(TAG, "Starting Setup Mode...");

            // Mount Filesystem to serve the Web UI
            if(!LittleFS.begin(true)){
                 _soc.getLogger().e(TAG, "LittleFS Mount Failed");
            } else {
                 _soc.getLogger().i(TAG, "LittleFS Mounted");
            }

            WiFi.disconnect();
            WiFi.mode(WIFI_AP);
            WiFi.softAP(_apSSID, _apPassword);

            IPAddress IP = WiFi.softAPIP();
            _soc.getLogger().i(TAG, "AP Started. IP: %s", IP.toString().c_str());

            _server = std::make_unique<AsyncWebServer>(80);
            _setupRoutes();
            
            _server->begin();
            _isSetupMode = true;
        }

        void Esp32WifiManager::stopSetupMode()
        {
            if (!_isSetupMode) return;

            _soc.getLogger().i(TAG, "Stopping Setup Mode...");
            if (_server) {
                _server->end();
            }
            
            WiFi.softAPdisconnect(true);
            WiFi.mode(WIFI_OFF);
            _isSetupMode = false;
        }

        void Esp32WifiManager::_setupRoutes()
        {
            // 1. Serve Static Files (The Svelte App)
            // 'serveStatic' automatically handles gzip files if they exist (e.g. index.html.gz)
            _server->serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

            // 2. API: Scan Networks
            _server->on("/api/wifi/scan", HTTP_GET, [this](AsyncWebServerRequest *request){
                _soc.getLogger().i(TAG, "API: Scan requested");
                
                // Synchronous scan (blocks for a moment)
                int n = WiFi.scanNetworks();
                
                JsonDocument doc;
                JsonArray array = doc.to<JsonArray>();
                
                for (int i = 0; i < n; ++i) {
                    JsonObject net = array.add<JsonObject>();
                    net["ssid"] = WiFi.SSID(i);
                    net["rssi"] = WiFi.RSSI(i);
                    // Check if enc type is OPEN
                    net["protected"] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
                }
                
                String response;
                serializeJson(doc, response);
                request->send(200, "application/json", response);
            });

            // 3. API: Save Credentials
            // Handler for the Request (sends the response)
            _server->on("/api/wifi/connect", HTTP_POST, 
                [](AsyncWebServerRequest *request){
                    request->send(200, "application/json", "{\"status\":\"ok\"}");
                },
                NULL,
                // Handler for the Body (receives the JSON)
                [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
                    
                    JsonDocument doc;
                    DeserializationError error = deserializeJson(doc, data, len);

                    if (!error) {
                        String ssid = doc["ssid"];
                        String pass = doc["password"];
                        
                        _soc.getLogger().i(TAG, "API: Saving credentials for %s", ssid.c_str());
                        
                        _preferences.begin("wifi-config", false);
                        _preferences.putString("ssid", ssid);
                        _preferences.putString("pass", pass);
                        _preferences.end();
                    } else {
                        _soc.getLogger().e(TAG, "API: Failed to parse credentials JSON");
                    }
            });
            
            // 4. API: Reboot
            _server->on("/api/system/reboot", HTTP_POST, [this](AsyncWebServerRequest *request){
                request->send(200, "application/json", "{\"status\":\"rebooting\"}");
                // Reboot after a short delay to allow response to be sent
                // We can't delay here, so we would need a timer or task.
                // For now, simple restart (might kill connection abruptly)
                delay(500);
                ESP.restart();
            });
            
            // 5. API: Status
            _server->on("/api/system/status", HTTP_GET, [this](AsyncWebServerRequest *request){
                 JsonDocument doc;
                 doc["ip"] = WiFi.localIP().toString();
                 doc["connected"] = (WiFi.status() == WL_CONNECTED);
                 
                 String response;
                 serializeJson(doc, response);
                 request->send(200, "application/json", response);
            });
        }

        std::string Esp32WifiManager::_scanNetworks() {
            // Unused in new async architecture, but kept if needed by interface? 
            // Actually it was private, so we can remove it.
            return ""; 
        }

    }
}
#endif // ESP32

