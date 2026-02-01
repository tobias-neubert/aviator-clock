#include "wifi/esp32/Esp32WifiManager.h"

#ifdef ESP32

#include <Arduino.h>
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
            // Open in R/W mode. If the namespace doesn't exist, it will be created.
            // This prevents "nvs_open failed: NOT_FOUND" errors.
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
                
                // Ensure clean state
                WiFi.disconnect();
                WiFi.persistent(false); // Do not use SDK's automatic storage 
                WiFi.mode(WIFI_STA);
                WiFi.setSleep(false); // Improve reliability
                delay(100);
                
                WiFi.begin(ssid.c_str(), pass.c_str());
                // We don't block here. Connection happens in background.
            }
            else
            {
                _soc.getLogger().i(TAG, "No credentials saved.");
            }
        }

        bool Esp32WifiManager::hasCredentials() const
        {
            Preferences p;
            // Open in R/W mode to ensure namespace exists and avoid error logs.
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
            WiFi.disconnect();
            WiFi.mode(WIFI_AP);
            WiFi.softAP(_apSSID, _apPassword);

            IPAddress IP = WiFi.softAPIP();
            _soc.getLogger().i(TAG, "AP Started. IP: %s", IP.toString().c_str());

            _server = std::make_unique<WebServer>(80);

            _server->on("/", HTTP_GET, [this]() {
                _handleRoot();
            });
            _server->on("/connect", HTTP_POST, [this]() {
                _handleSave();
            });

            _server->begin();
            _isSetupMode = true;
            _shouldStopSetup = false;

            // Start a task to handle web server clients
            // Lambda wrapper for the member function
            auto taskFunc = [this]() {
                this->_setupModeTask();
            };
            
            _webTask = _soc.createTask(
                taskFunc, 
                "WifiSetupTask", 
                4096, 
                1 // Low priority
            );
        }

        void Esp32WifiManager::stopSetupMode()
        {
            if (!_isSetupMode) return;

            _soc.getLogger().i(TAG, "Stopping Setup Mode...");
            _shouldStopSetup = true; // Signal task to stop
            
            // _webTask will exit its loop, clean up, and reset _isSetupMode.
            // We just let it happen asynchronously or we could join if we had that capability.
        }

        void Esp32WifiManager::_setupModeTask()
        {
            const unsigned long timeoutMs = 3 * 60 * 1000; // 3 minutes
            unsigned long startTime = millis();

            while (!_shouldStopSetup)
            {
                if (_server) {
                    _server->handleClient();
                }

                if (millis() - startTime > timeoutMs) {
                    _soc.getLogger().i(TAG, "Setup Mode timed out.");
                    _shouldStopSetup = true;
                }

                delay(10); // Yield
            }

            // Cleanup after setup mode (either saved or user toggled/timeout)
            _soc.getLogger().i(TAG, "Setup Mode task ending. Switching to STA...");
            
            if (_server) {
                _server->stop();
            }

            WiFi.softAPdisconnect(true);
            WiFi.disconnect(true); // Clear everything
            WiFi.mode(WIFI_OFF);   // Full radio reset
            
            delay(500); // Give radio a moment to reset
            
            WiFi.mode(WIFI_STA); 
            
            delay(100); 
            
            // Validate credentials existence before trying to connect
            if (hasCredentials()) {
                 begin();
            } else {
                _soc.getLogger().w(TAG, "Setup ended but no credentials. Staying offline.");
            }

            _isSetupMode = false; // Now we are officially done.
            _webTask.reset(); // Suicide / Detach? Actually dangerous if we are running IN it.
            // But usually we just let the function return. The unique_ptr in main class owns this task handle.
            // If we just return, the task enters "Deleted" state in FreeRTOS usually if not infinite loop.
            // However, our `taskRunner` does vTaskDelete(NULL). So just returning is correct.
            
            // BUT: `_webTask` in class still holds the handle. 
            // It's a `std::unique_ptr<ITask>`. Ideally we should clear it, but we can't clear ourselves easily.
            // It's fine, next `startSetupMode` will overwrite it.
        }

        void Esp32WifiManager::_handleRoot() {
             String html = "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
             // ... same html ...
             html += "<title>Aviator Clock Setup</title>";
             html += "<style>body{font-family:sans-serif;padding:20px;max-width:400px;margin:0 auto;} h2{text-align:center;} input,select,button{width:100%;margin:10px 0;padding:10px;box-sizing:border-box;}</style>";
             html += "</head><body><h2>WiFi Setup</h2>";
             
             html += "<form action=\"/connect\" method=\"POST\" autocomplete=\"off\">";
             html += "<label>SSID:</label>";
             
             // Scan for networks
             String options = _scanNetworks().c_str();
             html += "<select name=\"ssid\">" + options + "</select>";
             
             html += "<label>Password:</label><input type=\"password\" name=\"pass\" autocomplete=\"new-password\">";
             html += "<button type=\"submit\">Connect</button>";
             html += "</form>";
             html += "</body></html>";
             
             _server->send(200, "text/html", html);
        }

        void Esp32WifiManager::_handleSave() {
            if (!_server->hasArg("ssid") || !_server->hasArg("pass")) {
                _server->send(400, "text/plain", "Missing data");
                return;
            }

            String ssid = _server->arg("ssid");
            String pass = _server->arg("pass");
            
            // Trim whitespace to prevent copy-paste errors
            ssid.trim();
            pass.trim();

            _soc.getLogger().i(TAG, "Saving credentials for '%s' (Length: %d)", ssid.c_str(), ssid.length());
            _soc.getLogger().i(TAG, "Password length: %d", pass.length());

            _preferences.begin("wifi-config", false);
            _preferences.putString("ssid", ssid);
            _preferences.putString("pass", pass);
            _preferences.end();
            
            // Send success page BEFORE killing the server
            String html = "<!DOCTYPE html><html><body><h2>Saved!</h2><p>Connecting to " + ssid + "...</p></body></html>";
            _server->send(200, "text/html", html);
            
            delay(1000); // Give time to flush response
            
            _shouldStopSetup = true; // Signal task to stop
        }

        std::string Esp32WifiManager::_scanNetworks() {
            int n = WiFi.scanNetworks();
            std::string options = "";
            if (n == 0) {
                options = "<option>No networks found</option>";
            } else {
                // Deduplicate SSIDs, keeping the one with the best RSSI
                std::map<String, int> bestIndices;
                for (int i = 0; i < n; ++i) {
                    String ssid = WiFi.SSID(i);
                    if (ssid.length() == 0) continue; // Skip hidden/empty

                    if (bestIndices.find(ssid) == bestIndices.end()) {
                        bestIndices[ssid] = i;
                    } else {
                        // Compare RSSI, keep the higher one (closest to 0)
                        if (WiFi.RSSI(i) > WiFi.RSSI(bestIndices[ssid])) {
                             bestIndices[ssid] = i;
                        }
                    }
                }

                for (auto const& item : bestIndices) {
                    int i = item.second;
                    options += "<option value=\"" + std::string(WiFi.SSID(i).c_str()) + "\">" + 
                               std::string(WiFi.SSID(i).c_str()) + " (" + std::to_string(WiFi.RSSI(i)) + " dBm)</option>";
                }
            }
            return options;
        }

    }
}
#endif // ESP32
