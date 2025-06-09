#include <Arduino.h>
#include <memory>

// --- Framework/SoC Includes ---
#include <soc/api/ISocComponent.h>
#include <soc/esp32/ESP32MillisTime.h>
#include <soc/esp32/ESP32Logger.h>
#include <soc/esp32/ESP32Logger.h>

// --- Stepper Includes ---
#include <stepper/api/IStepperController.h>
#include <stepper/api/IHomingStrategy.h>
#include <stepper/accel/AccelStepperWrapper.h>
#include <stepper/accel/AccelStepperMotor.h>
#include <stepper/homing/LimitSwitchHomingStrategy.h>
#include <soc/esp32/ESP32DigitalInput.h>

// --- Application Includes ---
#include <aviator-clock/ClockHand.h>

// --- Pin Definitions ---
#define DIR_PIN_HW 12
#define STEP_PIN_HW 14
#define ENABLE_PIN_HW 27
#define LIMIT_SWITCH_PIN_HW 26

// --- Motor Configuration ---
const int EFFECTIVE_STEPS_PER_REVOLUTION = 1600;

// --- Clock Face Configuration ---
const double DIAL_TOTAL_ACTIVE_ANGLE = 330.0;
const double DIAL_START_OFFSET_DEGREES = 0.0;

// --- Settings for Sharper Movement ---
const double SHARP_TICK_SPEED_DPS = 2400.0;
const double SHARP_TICK_ACCELERATION_DPS2 = 60000.0;

// =========================================================================
// --- GLOBAL DECLARATIONS ---
// Declare all components as uninitialized smart pointers to their INTERFACES.
// This decouples our application from concrete implementations.
// see "The Static Initialization Order Fiasco"
// =========================================================================
std::unique_ptr<soc::api::ILogger> logger;
std::unique_ptr<soc::api::ITime> timeProvider;
std::unique_ptr<stepper::api::IStepperController> accelWrapper;
std::unique_ptr<soc::api::IDigitalInput> limitSwitch;
std::unique_ptr<stepper::api::IHomingStrategy> homingStrategy;
std::unique_ptr<IStepperMotor> motor;
std::unique_ptr<soc::api::ISocComponent> clockHand;

// --- State Flags of the main program---
bool clockOperationSetupDone = false;
bool failureLogged = false;

// --- Homing Configuration ---
const stepper::homing::LimitSwitchHomingStrategy::Config homingConfig = {
    .homingSpeedStepsPerSec = 400,                              // Slower speed for homing (e.g., 1/4 revolution per sec if 1600 steps/rev)
    .homingAccelerationStepsPerSecSq = 800,                     // Gentle acceleration
    .maxHomingTravelSteps = EFFECTIVE_STEPS_PER_REVOLUTION * 2, // e.g., allow 2 full revolutions to find switch
    .moveDirectionSign = -1                                     // IMPORTANT: Set this to +1 or -1 depending on your setup!
                                                                // -1 usually means counter-clockwise for AccelStepper
};

void setup()
{
  Serial.begin(115200);
  // =========================================================================
  // --- INSTANTIATION AND WIRING (Dependency Injection) ---
  // Create all objects in a controlled order, satisfying dependencies.
  // This is our "Composition Root".
  // see "The Static Initialization Order Fiasco"
  // =========================================================================
  logger = std::make_unique<soc::esp32::ESP32Logger>(soc::api::ILogger::INFO_LEVEL);
  timeProvider = std::make_unique<soc::esp32::ESP32MillisTime>();
  accelWrapper = std::make_unique<stepper::accel::AccelStepperWrapper>(STEP_PIN_HW, DIR_PIN_HW);
  limitSwitch = std::make_unique<soc::esp32::ESP32DigitalInput>(LIMIT_SWITCH_PIN_HW, true);
  limitSwitch->begin();

  // Now we can use the logger
  logger->info("=================================================");
  logger->info(" ESP32 Aviator Clock");
  logger->info(" System Booted: %s, %s", __DATE__, __TIME__);
  logger->info("=================================================");
  logger->info("Level 0 components created.");

  // Pass dependencies by reference by DEREFERENCING the smart pointers with *.
  homingStrategy = std::make_unique<stepper::homing::LimitSwitchHomingStrategy>(
      *accelWrapper,
      *limitSwitch,
      homingConfig,
      *logger);
  logger->info("Level 1 components (HomingStrategy) created.");

  motor = std::make_unique<stepper::accel::AccelStepperMotor>(
      *accelWrapper,
      EFFECTIVE_STEPS_PER_REVOLUTION,
      *homingStrategy,
      *logger,
      ENABLE_PIN_HW,
      true // enablePinActiveLow
  );
  logger->info("Level 2 components (Motor) created.");

  clockHand = std::make_unique<aviator_clock::ClockHand>(
      aviator_clock::ClockHand::HandType::SECOND,
      *timeProvider,
      std::move(motor),
      *logger,
      DIAL_TOTAL_ACTIVE_ANGLE,
      DIAL_START_OFFSET_DEGREES,
      SHARP_TICK_SPEED_DPS,
      SHARP_TICK_ACCELERATION_DPS2);
  logger->info("Level 3 components (ClockHands) created.");
  logger->info("All components created and wired up successfully.");

  // --- Now, proceed with operational logic using the initialized objects ---
  // simply setup the root component, it will setup its children
  if (clockHand)
  {
    clockHand->setup();
  }
}

void loop()
{
  {
    // do not check for the motor, because its not owned anymore
    // by the main program. It has been moved to the clock.
    // see The Static Initialization Order Fiasco
    if (!logger || !clockHand)
    {
      Serial.printf("Objects not initialized correctly\n");
      return;
    }

    clockHand->advanceState(millis());
    clockHand->render();
  }
}