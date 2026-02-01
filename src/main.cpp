#include <Arduino.h>
#include <memory>
#include <soc/api/Logger.h>

// --- Framework/SoC Includes ---
#include <soc/esp32/ESP32Soc.h>
#include <wifi/esp32/Esp32WifiManager.h>

// --- Application Includes ---
#include "AviatorClock.h"

// =========================================================================
// --- GLOBAL DECLARATIONS ---
// =========================================================================
std::unique_ptr<soc::api::ISoc> g_soc;
std::unique_ptr<wifi::api::IWifiManager> g_wifiManager;
std::unique_ptr<aviator_clock::AviatorClock> g_aviatorClock;

// =========================================================================
// --- SETUP & LOOP ---
// =========================================================================
void setup()
{
  Serial.begin(115200);

  // 1. Create and initialize the central SoC object
  auto esp32Soc = std::make_unique<soc::esp32::ESP32Soc>();
  esp32Soc->init();
  g_soc = std::move(esp32Soc);

  // Set the log level to INFO (ignoring DEBUG messages)
  g_soc->getLogger().setLogLevel(soc::api::LogLevel::INFO);

  g_soc->getLogger().i("main", "=================================================");
  g_soc->getLogger().i("main", " ESP32 Aviator Clock (RTOS Version)");
  g_soc->getLogger().i("main", " System Booted: %s, %s", __DATE__, __TIME__);
  g_soc->getLogger().i("main", "=================================================");
  g_soc->getLogger().i("main", "SoC created and initialized.");

  // 2. Create the WiFi Manager
  g_wifiManager = std::make_unique<wifi::esp32::Esp32WifiManager>(*g_soc);

  // 3. Create the main application object
  g_aviatorClock = std::make_unique<aviator_clock::AviatorClock>(*g_soc, *g_wifiManager);

  // 4. Setup and start the application
  g_aviatorClock->setup();
  g_aviatorClock->start();
}

void loop()
{
  // THIS IS INTENTIONALLY LEFT EMPTY.
  // The FreeRTOS scheduler is now in control. All logic runs in tasks.
}