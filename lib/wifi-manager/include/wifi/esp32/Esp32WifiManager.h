#pragma once

#ifdef ESP32

#include "wifi/api/IWifiManager.h"
#include <soc/api/ISoc.h>

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <memory>
#include <vector>

namespace wifi
{
    namespace esp32
    {
        class Esp32WifiManager : public wifi::api::IWifiManager
        {
        public:
            Esp32WifiManager(soc::api::ISoc& soc);
            virtual ~Esp32WifiManager();

            void begin() override;
            void startSetupMode() override;
            void stopSetupMode() override;
            bool isSetupModeActive() const override;
            bool hasCredentials() const override;
            bool isConnected() const override;
            void resetCredentials() override;

        private:
            void _setupModeTask(); // The task function
            void _handleWebClient();
            void _handleRoot();
            void _handleSave();
            std::string _scanNetworks();

        private:
            soc::api::ISoc& _soc;
            std::unique_ptr<WebServer> _server;
            std::unique_ptr<soc::api::ITask> _webTask;
            Preferences _preferences;
            
            // _isSetupMode indicates that the Setup Mode is logically active (AP running, Task running).
            // It remains true until cleanup is fully complete.
             volatile bool _isSetupMode = false;
             // _shouldStopSetup indicates that the user or logic requested to stop setup.
             volatile bool _shouldStopSetup = false;
            
            // Configuration for AP
            const char* _apSSID = "Aviator-Clock-Setup";
            const char* _apPassword = nullptr; // Open network
        };
    }
}
#endif // ESP32
