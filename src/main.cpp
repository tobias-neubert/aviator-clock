#include <Arduino.h>
#include <memory>
#include <soc/api/Logger.h>

// --- Framework/SoC Includes ---
#include <soc/esp32/ESP32Soc.h>
#include <wifi/esp32/Esp32WifiManager.h>

// --- Application Includes ---
#include "AviatorClock.h"

// =========================================================================
// --- GLOBAL INSTANCES (LIFECYCLE MANAGEMENT) ---
// =========================================================================
/*
 * WHY GLOBALS?
 * Unlike Java, C++ executes 'setup()' and then exits that function scope.
 * Local variables created in 'setup()' would be destroyed (popped off the stack)
 * when 'setup()' finishes.
 *
 * To ensure our core application components survive for the entire lifetime
 * of the device, we store them in global smart pointers.
 * 'std::unique_ptr' ensures they are the sole owners of the memory, similar to
 * a final reference in Java held by a container.
 */
std::unique_ptr<soc::api::ISoc> globalSystemOnChip;
std::unique_ptr<wifi::api::IWifiManager> globalWifiManager;
std::unique_ptr<aviator_clock::AviatorClock> globalAviatorClockApplication;

// =========================================================================
// --- SETUP & LOOP ---
// =========================================================================
/**
 * @brief System Initialization
 *
 * This function is called once by the Arduino framework when the device boots.
 * It acts as the "Bootstrap" phase of our application.
 */
void setup()
{
  Serial.begin(115200);

  // 1. Initialize Hardware Abstraction Layer (SoC)
  // We use the concrete ESP32 implementation here. This allows us to swap logic
  // for tests (using a MockSoc) without changing the application code.
  auto esp32SystemOnChip = std::make_unique<soc::esp32::ESP32Soc>();
  esp32SystemOnChip->init();
  
  // Transfer ownership to the global pointer so it survives setup()
  globalSystemOnChip = std::move(esp32SystemOnChip);

  // Configure Logging
  globalSystemOnChip->getLogger().setLogLevel(soc::api::LogLevel::INFO);
  globalSystemOnChip->getLogger().i("main", "=================================================");
  globalSystemOnChip->getLogger().i("main", " ESP32 Aviator Clock (RTOS Version)");
  globalSystemOnChip->getLogger().i("main", " System Booted: %s, %s", __DATE__, __TIME__);
  globalSystemOnChip->getLogger().i("main", "=================================================");
  
  // 2. Initialize Infrastructure Services
  // The WifiManager needs access to SoC services (logging, etc.)
  globalWifiManager = std::make_unique<wifi::esp32::Esp32WifiManager>(*globalSystemOnChip);

  // 3. Initialize Application Composition Root
  // The 'AviatorClock' class encapsulates the entire application logic.
  // We inject dependencies (SoC, WiFi) via constructor (Dependency Injection).
  globalAviatorClockApplication = std::make_unique<aviator_clock::AviatorClock>(
      *globalSystemOnChip, 
      *globalWifiManager
  );

  // 4. Start Application Lifecycle
  globalAviatorClockApplication->setup();
  globalAviatorClockApplication->start();
}

/**
 * @brief Main Execution Loop
 *
 * In a traditional Arduino sketch, this would contain the main logic.
 *
 * WHY IS IT EMPTY?
 * We are using FreeRTOS (Real-Time Operating System), which is part of the ESP32 framework.
 * The 'AviatorClock::start()' method has likely spawned independent Tasks (threads) 
 * for the motors, WiFi, and application logic.
 * 
 * When setup() finishes, the FreeRTOS scheduler takes over and manages
 * the tasks. This 'loop' task becomes just an idle task and is typically not needed
 * in a pure RTOS design.
 * 
 * We call 'vTaskDelete(NULL)' to delete this task and free up its resources, 
 * since we don't need it. That means, the loop() function is called once, 
 * and then the task is deleted, leaving only our custom tasks running.
 */
void loop()
{
  // Intentionally left empty. Logic runs in FreeRTOS tasks.
  // We delete this task to free up the memory allocated for the loop stack.
  vTaskDelete(NULL); 
}
