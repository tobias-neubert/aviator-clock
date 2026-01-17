#include <Arduino.h>
#include <memory>

// --- Framework/SoC Includes ---
#include <soc/esp32/ESP32Soc.h>

// --- Stepper Includes ---
#include <stepper/api/IStepperController.h>
#include <stepper/api/IHomingStrategy.h>
#include <stepper/accel/AccelStepperWrapper.h>
#include <stepper/accel/AccelStepperMotor.h>
#include <stepper/homing/LimitSwitchHomingStrategy.h>

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
// =========================================================================
std::unique_ptr<soc::api::ISoc> g_soc;
std::unique_ptr<stepper::api::IStepperController> accelWrapper;
std::unique_ptr<soc::api::IDigitalInput> limitSwitch;
std::unique_ptr<stepper::api::IHomingStrategy> homingStrategy;
std::unique_ptr<aviator_clock::ClockHand> clockHand;

// --- Homing Configuration ---
const stepper::homing::LimitSwitchHomingStrategy::Config homingConfig = {
    .homingSpeedStepsPerSec = 400,
    .homingAccelerationStepsPerSecSq = 800,
    .maxHomingTravelSteps = EFFECTIVE_STEPS_PER_REVOLUTION * 2,
    .moveDirectionSign = -1};

void setup()
{
  Serial.begin(115200);

  // 1. Create and initialize the central SoC object
  auto esp32Soc = std::make_unique<soc::esp32::ESP32Soc>();
  esp32Soc->init("9zP4UWzi8xYmIG8c7VAk", "10504184df2b662bff3d4c2f97a8c99f");
  g_soc = std::move(esp32Soc);

  auto &logger = g_soc->getLogger();
  logger.info("=================================================");
  logger.info(" ESP32 Aviator Clock (RTOS Version)");
  logger.info(" System Booted: %s, %s", __DATE__, __TIME__);
  logger.info("=================================================");
  logger.info("SoC created and initialized.");

  // 2. Create hardware-related components
  accelWrapper = std::make_unique<stepper::accel::AccelStepperWrapper>(STEP_PIN_HW, DIR_PIN_HW);
  limitSwitch = g_soc->createDigitalInput(LIMIT_SWITCH_PIN_HW, true);

  logger.info("Level 0 components created.");

  homingStrategy = std::make_unique<stepper::homing::LimitSwitchHomingStrategy>(
      *accelWrapper, *limitSwitch, homingConfig, logger);

  logger.info("Level 1 components (HomingStrategy) created.");

  auto motor = std::make_unique<stepper::accel::AccelStepperMotor>(
      *accelWrapper, EFFECTIVE_STEPS_PER_REVOLUTION, *homingStrategy, logger,
      ENABLE_PIN_HW, true);

  logger.info("Level 2 components (Motor) created.");

  // 3. Create the main application component (ClockHand)
  // The ClockHand is now an active component that manages its own task.
  clockHand = std::make_unique<aviator_clock::ClockHand>(
      aviator_clock::ClockHand::HandType::SECOND,
      *g_soc,
      std::move(motor),
      DIAL_TOTAL_ACTIVE_ANGLE,
      DIAL_START_OFFSET_DEGREES,
      SHARP_TICK_SPEED_DPS,
      SHARP_TICK_ACCELERATION_DPS2);

  logger.info("Level 3 components (ClockHand) created.");

  // 4. Setup and start the application component
  if (clockHand)
  {
    clockHand->setup();
    clockHand->start();
    logger.info("ClockHand task started.");
  }
}

void loop()
{
  // THIS IS INTENTIONALLY LEFT EMPTY.
  // The FreeRTOS scheduler is now in control. All logic runs in tasks.
}