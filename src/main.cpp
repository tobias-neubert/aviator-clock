#include <Arduino.h>
#include <memory>
#include "esp_log.h"

// --- Framework/SoC Includes ---
#include <soc/esp32/ESP32Soc.h>

// --- Application Includes ---
#include "AviatorClock.h"

// =========================================================================
// --- GLOBAL DECLARATIONS ---
// =========================================================================
static const char* TAG = "main";
std::unique_ptr<soc::api::ISoc> g_soc;
std::unique_ptr<aviator_clock::AviatorClock> g_aviatorClock;

// =========================================================================
// --- SETUP & LOOP ---
// =========================================================================
void setup()
{
  Serial.begin(115200);

  // 1. Create and initialize the central SoC object
  auto esp32Soc = std::make_unique<soc::esp32::ESP32Soc>();
  esp32Soc->init("Starcommand", "75v5VrZSyKRQatFtkRmV");
  g_soc = std::move(esp32Soc);

  ESP_LOGI(TAG, "=================================================");
  ESP_LOGI(TAG, " ESP32 Aviator Clock (RTOS Version)");
  ESP_LOGI(TAG, " System Booted: %s, %s", __DATE__, __TIME__);
  ESP_LOGI(TAG, "=================================================");
  ESP_LOGI(TAG, "SoC created and initialized.");

  // 2. Create the main application object
  g_aviatorClock = std::make_unique<aviator_clock::AviatorClock>(*g_soc);

  // 3. Setup and start the application
  g_aviatorClock->setup();
  g_aviatorClock->start();
}

void loop()
{
  // THIS IS INTENTIONALLY LEFT EMPTY.
  // The FreeRTOS scheduler is now in control. All logic runs in tasks.
}