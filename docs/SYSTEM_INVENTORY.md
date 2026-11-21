# System Inventory: Aviator Clock

This document serves as an exhaustive, factual inventory of the Aviator Clock codebase for AI agents. Its purpose is to equip agents to write specifications, structure tickets, formulate grilling questions, implement features, and maintain documentation without guessing or assuming hidden context.

---

## 1. System Overview & Physical Architecture

The **Aviator Clock** is an ESP32-based desktop clock inspired by classic analog aircraft flight instruments.

- **Dials & Hands**: The production hardware features three separate analog dials on the clock face (Hours, Minutes, Seconds). Each dial is driven by a dedicated bipolar stepper motor through a step/direction driver.
- **Microcontroller**: ESP32 (NodeMCU-32S board) running FreeRTOS on top of the Arduino-ESP32 framework.
- **Dial Geometry**: Active scale arc is configured to **330.0°** (rather than 360°), reflecting flight gauge styling with an inactive sector at the bottom/top.
- **Stepper Drive & Homing**: 
  - Stepper motor resolution: 1600 steps/revolution (200 full steps * 8 microsteps).
  - Zero-position sensing: A3144 unipolar digital Hall-effect sensor per hand with a neodymium magnet on the rotating indicator disc.
  - Electrical interface: 5V sensor module with a 10 kΩ / 10 kΩ voltage divider down to 2.5–3.3V for safe ESP32 3.3V GPIO inputs.
  - Homing strategy (`PreciseOneWayHomingStrategy`): One-way clockwise search; if starting on the sensor, back off until inactive; stop immediately upon sensor activation, set current position as zero point.
- **User Interface Inputs**:
  - **Setup Button**: Single push button for toggling WiFi Setup Mode (AP + Web UI) on a 3-second press and triggering factory reset on a 10-second hold.
- **Audio Output**:
  - **Sound Module**: JQ6500-16P MP3 module (1 off) for playing the configured alarm tone.
- **Web UI & Connectivity**:
  - Hosted Single Page Application (Svelte 4 + TypeScript) bundled and gzipped directly into LittleFS (`data/`).
  - Served via ESPAsyncWebServer on port 80 during Setup Mode (AP name: `Aviator-Clock-Setup`).
  - Manages both Wi-Fi connectivity and alarm configuration (time and sound selection).

---

## 2. Hardware Architecture & Scope for Version 1.0

| Subsystem | Components & Responsibilities |
| :--- | :--- |
| **Stepper Motors** | **3 physical stepper motors** (Hours, Minutes, Seconds) with STEP/DIR drivers. |
| **Zero Sensing** | **3 Hall sensors** (A3144 unipolar digital), one per hand for homing. |
| **Audio** | **1x JQ6500-16P MP3 Module** connected via UART for alarm sounds. |
| **Manual Controls** | **1x Setup Button** (GPIO 15) to enter Setup Mode / Factory Reset. |
| **Configuration** | Handled exclusively via the embedded **Web UI** (Wi-Fi, alarm time, sound selection). |

---

## 3. Hardware Pin Assignments (`src/platform/board_support_package/PinConfig.h`)

| Signal / Role | ESP32 GPIO | Electrical Characteristics / Constraints |
| :--- | :--- | :--- |
| `Hour Hand Step / Dir / Hall` | 19 / 18 / 35 | Output STEP/DIR to driver, Hall zero sensor input |
| `Minute Hand Step / Dir / Hall` | 27 / 26 / 34 | Output STEP/DIR to driver, Hall zero sensor input |
| `Second Hand Step / Dir / Hall` | 14 / 12 / 25 | Output STEP/DIR to driver, Hall zero sensor input |
| `PIN_MOTOR_ENABLE` | 33 | Output to stepper driver EN pin (Active LOW) |
| `PIN_SETUP_BUTTON` | 15 | Active LOW pushbutton (WiFi setup & factory reset) |
| `JQ6500-16P UART RX / TX` | 16 / 17 | Hardware Serial2 to MP3 sound module (9600 8N1) |

---

## 4. Software Architecture & Module Directory

The codebase follows the **Hexagonal Architecture (Ports & Adapters)** pattern and strict single module structure rooted under `src/`.

```text
.
├── src/
│   ├── main.cpp                              # Bootstrap entry point; transfers control to FreeRTOS scheduler
│   ├── app/                                  # Namespace: aviator::app
│   │   ├── CompositionRoot.h/.cpp            # Composition root: dependency injection & task lifecycle
│   │
│   ├── domain/                               # Namespace: aviator::domain (Pure C++17)
│   │   ├── DialGeometry.h/.cpp               # Dial angle calculations for 330.0° active arc
│   │   ├── ClockEngine.h/.cpp                # Translates time into dial hand targets & wrap-arounds
│   │   ├── AlarmCoordinator.h/.cpp           # Evaluates real-time clock and triggers/dismisses alarms
│   │   ├── ClockConfigurationService.h/.cpp  # Domain service implementing ClockConfigurationApi
│   │   ├── OperatingStateMachine.h/.cpp      # State machine (Initial, NormalRunning, SetupMode)
│   │   ├── Alarm.h                           # Plain data struct for alarm settings
│   │   ├── Time.h                            # Plain data struct for time components
│   │   │
│   │   │   # Ports (Domain boundary interfaces — NO 'I' prefix)
│   │   ├── HandActuator.h                    # Outbound actuator contract (home, move, isHomed)
│   │   ├── AlarmStorage.h                    # Outbound persistent storage contract
│   │   ├── TimeSource.h                      # Outbound time provider contract
│   │   ├── AudioPlayer.h                     # Outbound sound module playback contract
│   │   ├── SystemGateway.h                   # Outbound restart & credentials recovery contract
│   │   ├── ClockInputEvents.h                # Inbound hardware user interaction events
│   │   └── ClockConfigurationApi.h           # Inbound clock management interface for Web UI
│   │
│   └── platform/                             # Namespace: aviator::platform (ESP32 & FreeRTOS)
│       ├── board_support_package/
│       │   └── PinConfig.h                   # Hardware pin assignments and electrical parameters
│       ├── motor/
│       │   ├── StepperSubsystem.h/.cpp       # Single high-priority FreeRTOS motor runner task
│       │   ├── StepperHandActuator.h/.cpp    # Implementation of HandActuator port
│       │   ├── PreciseOneWayHomingStrategy.h/.cpp # Precision one-way homing with backoff
│       │   ├── AccelStepperWrapper.h         # Adapter wrapping third-party AccelStepper
│       │   ├── StepperController.h           # Motor driver abstraction interface
│       │   └── HomingStrategy.h              # Homing algorithm interface
│       ├── input/
│       │   └── SetupButtonAdapter.h/.cpp     # Inbound adapter monitoring single Setup Button (GPIO 15)
│       ├── storage/
│       │   └── NvsAlarmStorage.h/.cpp        # Implementation of AlarmStorage via Preferences (NVS)
│       ├── time/
│       │   └── NtpTimeSource.h/.cpp          # Implementation of TimeSource via SNTP & POSIX time
│       ├── audio/
│       │   └── Jq6500AudioPlayer.h/.cpp      # Implementation of AudioPlayer for JQ6500-16P MP3 module
│       ├── web/
│       │   └── WebPortalServer.h/.cpp        # AsyncWebServer + LittleFS configuration portal
│       ├── system/
│       │   └── EspSystemGateway.h            # Implementation of SystemGateway for ESP32
│       ├── sensor/
│       │   ├── DigitalInput.h                # Binary digital input interface
│       │   └── GpioDigitalInput.h            # Microcontroller GPIO pin reader
│       └── logging/
│           └── Logger.h                      # Tag-based diagnostic logging
├── web-client/                               # Svelte 4 SPA frontend source
├── data/                                     # LittleFS filesystem binary assets (gzipped HTML/JS/CSS)
├── test/
│   ├── gtests/                               # GoogleTest unit test suites (run on native host)
│   └── mocks/                                # Mock test doubles for native host unit tests
└── platformio.ini                            # PlatformIO build configuration
```

### Module Breakdown

1. **`src/main.cpp`**:
   - Instantiates `globalCompositionRoot` (`aviator::app::CompositionRoot`).
   - Invokes `setup()` and `start()`.
   - Calls `vTaskDelete(NULL)` in `loop()` so FreeRTOS scheduler handles all background tasks without an empty polling loop.
2. **`CompositionRoot` (`src/app/CompositionRoot.cpp`)**:
   - Assembles platform adapters and wires them to domain ports via dependency injection.
   - Manages FreeRTOS task lifecycles (`motor_runner_task`, `input_poller_task`, `clock_engine_task`).
   - Implements `ClockInputEvents` and `OperatingModeObserver`.
3. **`StepperSubsystem` (`src/platform/motor/StepperSubsystem.cpp`)**:
   - Owns a single high-priority FreeRTOS task (`motor_runner_task`, Core 1) that steps all three dial hands simultaneously.
4. **`SetupButtonAdapter` (`src/platform/input/SetupButtonAdapter.cpp`)**:
   - Background polling task monitoring GPIO 15 with debouncing:
     - Short press (< 3s): dismisses active ringing alarm.
     - Long press (3s - 10s): toggles Setup Mode (AP + Web UI).
     - Hold (>= 10s): triggers Factory Reset (erases NVS credentials and restarts).
5. **`NvsAlarmStorage` & `NtpTimeSource`**:
   - Direct integration with ESP32 NVS `Preferences` and ESP-IDF SNTP with POSIX timezone configuration.
6. **`Jq6500AudioPlayer`**:
   - Direct hardware UART driver for JQ6500-16P MP3 audio module.
7. **`WebPortalServer`**:
   - Hosts `ESPAsyncWebServer` on port 80, serves static gzipped files from LittleFS, and provides REST APIs for Wi-Fi and alarm configuration.
8. **`web-client`**:
   - Modern Svelte 4 SPA built with Vite and `vite-plugin-compression` compressing assets into `data/`.

---

## 5. Build, Test, and CI Environment

- **PlatformIO Environments**:
  - `nodemcu-32s`: Target ESP32 hardware build (framework: Arduino, C++17, LittleFS filesystem, `min_spiffs.csv` partition table).
  - `google_tests`: Native desktop test runner using GoogleTest 1.15.2.
  - `unity_tests`: Native desktop test runner using Unity 2.6.1.
- **Current Test Status**:
  - `pio test -e google_tests`: **17/17 tests passing** (covers `AccelStepperMotor`, `AlarmRepository`, `ClockHand`).
  - `pio test -e unity_tests`: **Fails to compile** due to missing `#include <cstdint>` in `lib/arduino-mock/include/MockSerial.h` and `lib/arduino-mock/include/Arduino.h` when compiled under native host GCC.

---

## 6. Identified Inconsistencies & Gaps (Ammunition for Grilling)

When agents review specifications, plan tickets, or grill the developer, the following tension points must be addressed:

1. **Missing Hands Implementation**:
   - The vision specifies 3 dials/hands. Currently, only `_secondHand` exists. `_hourHand` and `_minuteHand` are completely absent in `AviatorClock.cpp`.
2. **Breadboard Motor Conflict**:
   - In `KONZEPT_ALARM_SETUP.md` (Section 5), the plan prescribes mapping the single breadboard motor to the **Minute Hand** (for direct interactive feedback when turning the encoder). In `AviatorClock.cpp` line 374, it is instantiated as `_secondHand` (acting merely as an AM/PM toggle indicator between 0 and 30).
3. **Absence of Alarm Triggering Logic**:
   - The code saves and edits alarms in NVS via `AlarmRepository` and `TimeEditor`.
   - However, there is no code that compares the current time against saved alarms, no buzzer/visual alarm component, and no concept of how the user dismisses an alarm.
4. **Boot Offline Blocking vs. Specification**:
   - `KONZEPT_WIFI_SETUP.md` specifies that if WiFi/NTP is unavailable, the clock should fall back to an "offline mode" and run anyway.
   - `AviatorClock::start()` currently halts completely (`Initialization failed. Clock will NOT start.`) if WiFi connection or NTP synchronization times out.
5. **Dial Angle vs. Continuous Motion**:
   - The dial active angle is 330.0°. How does a hand transition from 59 to 0? Does it sweep across the 30° gap forward, or does it reverse?
6. **Rotary Encoder Pushbutton Mapping**:
   - The hardware sheet specifies a Dual Rotary Encoder with a built-in momentary switch.
   - The code defines two separate button pins: `PIN_SETUP_BUTTON` (GPIO 15) and `PIN_MODE_BUTTON` (GPIO 2). Which physical switch corresponds to which button?
7. **Task Stack Sizes & Concurrency Safety**:
   - `ClockHand` uses `_systemOnChip.createTask()` with an internal loop. When all 3 hands are running alongside `_inputMonitoringTask` and `AsyncWebServer`, FreeRTOS task priority, stack depths, and core affinity must be evaluated.
