#include <Arduino.h>
#include <memory>
#include "esp_log.h"

// --- Framework/SoC Includes ---
#include <soc/esp32/ESP32Soc.h>

// --- Stepper Includes ---
#include <stepper/api/IStepperController.h>
#include <stepper/api/IHomingStrategy.h>
#include <stepper/accel/AccelStepperWrapper.h>
#include <stepper/accel/AccelStepperMotor.h>
#include <stepper/homing/PreciseOneWayHomingStrategy.h>

// --- Button Includes ---
#include <buttons/impl/LongPressButton.h>

// --- Application Includes ---
#include <aviator-clock/ClockHand.h>

// --- Pin Definitions ---
#define DIR_PIN_HW 12
#define STEP_PIN_HW 14
#define HALL_SENSOR_PIN 25
#define ENABLE_PIN_HW 33
#define SETUP_BUTTON_PIN 15

// --- Motor Configuration ---
const int EFFECTIVE_STEPS_PER_REVOLUTION = 1600;

// --- Clock Face Configuration ---
const double DIAL_TOTAL_ACTIVE_ANGLE = 330.0;

// --- Settings for Smooth Movement ---
const double SHARP_TICK_SPEED_DPS = 360.0;
const double SHARP_TICK_ACCELERATION_DPS2 = 1000.0;

// =========================================================================
// --- GLOBAL DECLARATIONS ---
// =========================================================================
static const char* TAG = "main";
// smart pointers for automatic resource management
// unique_ptrs are used to ensure single ownership semantics, there can always be only
// one owner of each resource, which simplifies lifecycle management in an RTOS environment.
std::unique_ptr<soc::api::ISoc> g_soc;
std::unique_ptr<stepper::api::IStepperController> accelWrapper;
std::unique_ptr<soc::api::IDigitalInput> hallSensor;
std::unique_ptr<soc::api::IDigitalInput> setupButtonPin;
std::unique_ptr<IButton> setupButton;
std::unique_ptr<stepper::api::IHomingStrategy> homingStrategy;
std::unique_ptr<aviator_clock::ClockHand> clockHand;

// Function to check for setup mode activation at boot
bool checkForSetupMode() {
    ESP_LOGI(TAG, "Checking for setup mode activation...");
    ESP_LOGI(TAG, "Hold button on GPIO%d for 3s to enter setup.", SETUP_BUTTON_PIN);
    
    // Check for a few seconds at boot
    const unsigned long checkDuration = 4000; // ms
    unsigned long checkStartTime = g_soc->getTimeProvider().millis();

    while(g_soc->getTimeProvider().millis() - checkStartTime < checkDuration) {
        setupButton->update();
        if (setupButton->wasLongPressed()) {
            ESP_LOGW(TAG, "Long press detected! Entering setup mode.");
            return true;
        }
        if (setupButton->wasShortPressed()) {
            ESP_LOGI(TAG, "Short press detected. Continuing normal boot.");
            return false;
        }
        g_soc->getSleeper().sleep(10); // yield
    }
    
    ESP_LOGI(TAG, "No button press detected. Continuing normal boot.");
    return false;
}

void setup()
{
  Serial.begin(115200);

  // 1. Create and initialize the central SoC object
  // Best practice: Create and initialize a LOCAL pointer first and
  // move it into the GLOBAL pointer only after successful initialization.
  // Prevents having a global pointer in an invalid state.
  // auto: type inference
  auto esp32Soc = std::make_unique<soc::esp32::ESP32Soc>();
  esp32Soc->init("Starcommand", "75v5VrZSyKRQatFtkRmV");
  g_soc = std::move(esp32Soc);

  ESP_LOGI(TAG, "=================================================");
  ESP_LOGI(TAG, " ESP32 Aviator Clock (RTOS Version)");
  ESP_LOGI(TAG, " System Booted: %s, %s", __DATE__, __TIME__);
  ESP_LOGI(TAG, "=================================================");
  ESP_LOGI(TAG, "SoC created and initialized.");

  // Create hardware-related components
  hallSensor = g_soc->createDigitalInput(HALL_SENSOR_PIN, true /* isActiveLow */);
  hallSensor->begin();
  setupButtonPin = g_soc->createDigitalInput(SETUP_BUTTON_PIN, true /* isActiveLow */);
  setupButtonPin->begin();
  setupButton = std::make_unique<LongPressButton>(*g_soc, *setupButtonPin);

  // Check for setup mode activation
  if (checkForSetupMode()) {
    // Here we will eventually call the WiFi setup function
    // For now, we'll just indicate it and halt execution
    ESP_LOGI(TAG, "Setup mode activated. Further implementation needed.");
    while (true) {
      g_soc->getSleeper().sleep(1000);
    }
  }

  // 2. Create other hardware-related components
  accelWrapper = std::make_unique<stepper::accel::AccelStepperWrapper>(STEP_PIN_HW, DIR_PIN_HW);
  ESP_LOGI(TAG, "Level 0 components created.");

  // Configuration for the new Homing Strategy
  stepper::homing::PreciseOneWayHomingStrategy::Config homingConfig{
      .homingSpeed = 200.0f,                                   // Very slow speed for precision
      .homingAcceleration = 1000.0f,                           // Gentle acceleration
      .maxHomingTravel = EFFECTIVE_STEPS_PER_REVOLUTION * 2,  // Travel max 2 revolutions
      .moveDirection = 1,                                     // +1 for clockwise
      .backOffDistance = 100                                  // Steps to move if starting on switch
  };

  // Instantiate the new PreciseOneWayHomingStrategy
  homingStrategy = std::make_unique<stepper::homing::PreciseOneWayHomingStrategy>(
      *accelWrapper,
      *hallSensor,
      homingConfig);

  ESP_LOGI(TAG, "Level 1 components (HomingStrategy) created.");

  // The enable pin is now passed to the constructor to allow for software control.
  auto motor = std::make_unique<stepper::accel::AccelStepperMotor>(
      *accelWrapper,
      EFFECTIVE_STEPS_PER_REVOLUTION,
      *homingStrategy,
      ENABLE_PIN_HW);

  ESP_LOGI(TAG, "Level 2 components (Motor) created.");

  // 3. Create the main application component (ClockHand)
  // The ClockHand is now an active component that manages its own task.
  clockHand = std::make_unique<aviator_clock::ClockHand>(
      aviator_clock::ClockHand::HandType::SECOND,
      *g_soc,
      std::move(motor),
      DIAL_TOTAL_ACTIVE_ANGLE,
      SHARP_TICK_SPEED_DPS,
      SHARP_TICK_ACCELERATION_DPS2);

  ESP_LOGI(TAG, "Level 3 components (ClockHand) created.");

  // 4. Setup and start the application component
  if (clockHand)
  {
    clockHand->setup();
    clockHand->start();
    ESP_LOGI(TAG, "ClockHand task started.");
  }
}

void loop()
{
  // THIS IS INTENTIONALLY LEFT EMPTY.
  // The FreeRTOS scheduler is now in control. All logic runs in tasks.
}