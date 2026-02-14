#pragma once

#ifdef ESP32

#include "wifi/api/IWifiManager.h"
#include <soc/api/ISoc.h>

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
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
            bool connect(unsigned long timeoutMs) override;
            void startSetupMode() override;
            void stopSetupMode() override;
            bool isSetupModeActive() const override;
            bool hasCredentials() const override;
            bool isConnected() const override;
            void resetCredentials() override;

        private:
            void _setupRoutes();
            std::string _scanNetworks();

            /**
             * @brief Checks if the time elapsed since _connectionStartTime exceeds the timeout.
             */
            bool _hasTimedOut(unsigned long timeoutMs) const;

        private:
            soc::api::ISoc& _soc;
            std::unique_ptr<AsyncWebServer> _server;
            Preferences _preferences;
            
            // State
            unsigned long _connectionStartTime{0};
            
            // _isSetupMode indicates that the Setup Mode is logically active (AP running, Server running).
             volatile bool _isSetupMode = false;
            
            // Configuration for AP
            const char* _apSSID = "Aviator-Clock-Setup";
            const char* _apPassword = nullptr; // Open network
        };
    }
}
#endif // ESP32
