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

// --- Application Includes ---
#include <aviator-clock/ClockHand.h>

// --- Pin Definitions ---
#define DIR_PIN_HW 12
#define STEP_PIN_HW 14
#define HALL_SENSOR_PIN 25

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
std::unique_ptr<stepper::api::IHomingStrategy> homingStrategy;
std::unique_ptr<aviator_clock::ClockHand> clockHand;

void setup()
{
  Serial.begin(115200);

  // 1. Create and initialize the central SoC object
  // Best practice: Create and initialize a LOCAL pointer first and
  // movie it into the GLOBAL pointer only after successful initialization.
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

  // 2. Create hardware-related components
  accelWrapper = std::make_unique<stepper::accel::AccelStepperWrapper>(STEP_PIN_HW, DIR_PIN_HW);
  // Create the digital input for our Hall sensor, configured as active low
  hallSensor = g_soc->createDigitalInput(HALL_SENSOR_PIN, true /* isActiveLow */);


  ESP_LOGI(TAG, "Level 0 components created.");

  // Configuration for the new Homing Strategy
  stepper::homing::PreciseOneWayHomingStrategy::Config homingConfig{
      .homingSpeed = 50.0f,                                   // Very slow speed for precision
      .homingAcceleration = 125.0f,                           // Gentle acceleration
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

  // Note: The enable pin argument is omitted, so the library will not control any enable pin.
  // This matches the hardware setup where the driver's EN pin is permanently grounded.
  auto motor = std::make_unique<stepper::accel::AccelStepperMotor>(
      *accelWrapper,
      EFFECTIVE_STEPS_PER_REVOLUTION,
      *homingStrategy);

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