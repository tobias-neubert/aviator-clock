#pragma once

namespace wifi
{
    namespace api
    {
        /**
         * @brief Interface for managing WiFi connectivity and configuration.
         */
        class IWifiManager
        {
        public:
            virtual ~IWifiManager() = default;

            /**
             * @brief Initializes the WiFi subsystem.
             * Attempts to auto-connect if credentials are valid.
             */
            virtual void begin() = 0;

            /**
             * @brief Starts the Setup Mode (AP + WebServer).
             * This should disconnect current station and start AP.
             */
            virtual void startSetupMode() = 0;

            /**
             * @brief Stops the Setup Mode.
             * This should stop AP and try to reconnect to Station.
             */
            virtual void stopSetupMode() = 0;

            /**
             * @brief Checks if the device is currently in Setup Mode (AP active).
             * @return true if in setup mode.
             */
            virtual bool isSetupModeActive() const = 0;

            /**
             * @brief Checks if valid credentials (SSID) are stored in NVS.
             * @return true if credentials exist.
             */
            virtual bool hasCredentials() const = 0;
            
            /**
             * @brief Checks if the device is connected to a WiFi network.
             */
            virtual bool isConnected() const = 0;

            /**
             * @brief Clears stored WiFi credentials.
             */
            virtual void resetCredentials() = 0;
        };
    }
}
