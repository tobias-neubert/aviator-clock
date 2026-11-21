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
  - **Slot Selector**: 3 toggle/DIP switches producing a 3-bit binary code (0 to 7) to select one of eight persistent alarm slots.
  - **Dual Rotary Encoder**: Concentric dual encoder for adjusting minutes and hours independently.
  - **Push Buttons**: Mode button and Setup button used for toggling operational modes and factory reset.
- **Web UI & Connectivity**:
  - Hosted Single Page Application (Svelte 4 + TypeScript) bundled and gzipped directly into LittleFS (`data/`).
  - Served via ESPAsyncWebServer on port 80 during WiFi Setup Mode (AP name: `Aviator-Clock-Setup`).

---

## 2. Breadboard Setup vs. Production Target

| Subsystem | Breadboard / As-Is State | Production Target State |
| :--- | :--- | :--- |
| **Stepper Motors** | **1 physical stepper motor** connected. | **3 physical stepper motors** (Hours, Minutes, Seconds). |
| **Code Hand Instance** | Only `_secondHand` is instantiated in `AviatorClock.cpp`. | Three distinct `ClockHand` instances (`_hourHand`, `_minuteHand`, `_secondHand`). |
| **Active Hand Mapping** | Single motor mapped to the **second hand**. | Each hand mapped to its dedicated driver & Hall sensor GPIOs. |
| **Alarm Setup Display** | Second hand moves to 0 (AM) or 30 (PM); hour/minute hand updates are commented out (`TODO`). | Minute and hour hands move to the configured alarm time on the dial; second hand indicates AM/PM. |

---

## 3. Hardware Pin Assignments (`src/AviatorClock.h`)

| Signal / Role | ESP32 GPIO | Electrical Characteristics / Constraints |
| :--- | :--- | :--- |
| `PIN_STEPPER_STEP_PULSE` | 14 | Output to stepper driver STEP pin |
| `PIN_STEPPER_DIRECTION` | 12 | Output to stepper driver DIR pin |
| `PIN_MOTOR_ENABLE` | 33 | Output to stepper driver EN pin (Active LOW) |
| `PIN_HALL_EFFECT_SENSOR` | 25 | Input from Hall sensor voltage divider (Active LOW) |
| `PIN_SETUP_BUTTON` | 15 | Active LOW pushbutton (WiFi setup & factory reset) |
| `PIN_MODE_BUTTON` | 2 | Active LOW pushbutton (Alarm setup toggle) |
| `PIN_DIP_SWITCH_BIT_0` | 26 | Input (LSB of alarm slot selector, Active LOW) |
| `PIN_DIP_SWITCH_BIT_1` | 27 | Input (Middle bit of alarm slot selector, Active LOW) |
| `PIN_DIP_SWITCH_BIT_2` | 32 | Input (MSB of alarm slot selector, Active LOW) |
| `PIN_ENCODER_MINUTE_A` | 36 | Input-only pin (GPI) — **requires external pull-up** |
| `PIN_ENCODER_MINUTE_B` | 39 | Input-only pin (GPI) — **requires external pull-up** |
| `PIN_ENCODER_HOUR_A` | 5 | Strap pin (must be HIGH at boot) — internal pull-up |
| `PIN_ENCODER_HOUR_B` | 4 | GPIO with internal pull-up |

---

## 4. Software Architecture & Module Directory

The codebase follows the **Composition Root** pattern and strict layered abstraction.

```text
.
├── src/
│   ├── main.cpp                 # Bootstrap entry point; transfers control to FreeRTOS scheduler
│   ├── AviatorClock.h           # Composition Root & State Machine header
│   └── AviatorClock.cpp         # Component wiring, lifecycle management, input monitoring loop
├── lib/
│   ├── soc-api/                 # Hardware Abstraction Layer (HAL) interfaces
│   ├── soc-esp32/               # Concrete ESP32 implementations (FreeRTOS, Preferences, GPIO)
│   ├── soc-mock/                # Host test doubles for HAL
│   ├── stepper-api/             # Abstract stepper motor & homing interfaces
│   ├── accel-stepper-adapter/   # Wrapper for third-party AccelStepper library
│   ├── buttons/                 # Input abstractions: LongPressButton, SwitchGroup
│   ├── wifi-manager/            # IWifiManager, Esp32WifiManager (AsyncWebServer + LittleFS)
│   ├── aviator-clock/           # Domain logic: ClockHand, Time, TimeEditor, AlarmRepository
│   └── arduino-mock/            # Mock headers for native unity testing
├── web-client/                  # Svelte 4 SPA frontend source
├── data/                        # LittleFS filesystem binary assets (gzipped HTML/JS/CSS)
├── test/
│   ├── gtests/                  # GoogleTest unit test suites (run on native host)
│   └── unity-tests/             # Unity unit test suites (run on native host)
└── platformio.ini               # PlatformIO build configuration
```

### Module Breakdown

1. **`src/main.cpp`**:
   - Instantiates `globalSystemOnChip` (`ESP32Soc`), `globalWifiManager` (`Esp32WifiManager`), and `globalAviatorClockApplication` (`AviatorClock`).
   - Invokes `setup()` and `start()`.
   - Calls `vTaskDelete(NULL)` in `loop()` so FreeRTOS scheduler handles all background tasks without an empty polling loop.
2. **`AviatorClock` (`src/AviatorClock.cpp`)**:
   - Owns state machine (`State::Initial`, `State::NormalRunning`, `State::AlarmSetup`, `State::WifiSetup`).
   - Launches a dedicated FreeRTOS input monitoring task (`_inputMonitoringTask`, polling every 20ms).
   - Coordinates long-press timing:
     - Setup Button: 3 seconds = toggle WiFi Setup Mode; 10 seconds = Factory Reset (erases NVS credentials and restarts).
     - Mode Button: 3 seconds = toggle Alarm Setup Mode.
3. **`lib/soc-api` & `lib/soc-esp32`**:
   - Clean facade `ISoc` providing access to `ITime`, `ISleeper`, `Logger`, `IPersistentStorage`, `createTask()`, `createDigitalInput()`, `createDigitalOutput()`, `createRotaryEncoder()`.
   - `ESP32Task` uses `xTaskCreatePinnedToCore()` on FreeRTOS.
   - `ESP32PersistentStorage` wraps ESP32 `Preferences` (NVS).
4. **`lib/stepper-api` & `lib/accel-stepper-adapter`**:
   - `IStepperMotor`: High-level stepper contract (`home()`, `moveToAbsolute()`, `stop()`, `update()`, `getState()`).
   - `AccelStepperMotor`: State machine managing transitions across `Idle`, `HomingInProgress`, `HomingFailed`, `Moving`.
   - `PreciseOneWayHomingStrategy`: Implements sensor detection, overshoot handling, and zero calibration.
5. **`lib/buttons`**:
   - `LongPressButton`: Tracks press start, active duration, debounces, and provides `wasLongPressed()`.
   - `SwitchGroup`: Aggregates 3 `IDigitalInput` pins into a 3-bit binary integer (0–7).
6. **`lib/wifi-manager`**:
   - `Esp32WifiManager`: Handles STA connection, AP captive portal (`Aviator-Clock-Setup`), LittleFS static web serving, and REST endpoints:
     - `GET /api/wifi/scan`: Scans networks, returns deduplicated JSON with RSSI and encryption.
     - `POST /api/wifi/connect`: Writes `ssid` and `password` to NVS `wifi-config` namespace.
     - `GET /api/system/status`: Returns current IP and connection state.
     - `POST /api/system/reboot`: Reboots device.
     - `POST /api/setup/finish`: Exits AP mode after 1 second delay.
7. **`lib/aviator-clock`**:
   - `ClockHand`: Controls a single hand via its own FreeRTOS task loop. Translates target time units (0–59 or 0–11) into angular motor positions based on active dial angle. Supports `setup()`, `start()`, `stop()`, `resume()`, `moveToValue()`.
   - `Time`: Plain data struct `{ hours, minutes, seconds }`.
   - `TimeEditor`: Handles incremental modifications from encoders, handles 24h/60m wrap-around, and maps AM/PM to second hand position (0 = AM, 30 = PM).
   - `AlarmRepository`: Persists alarm times across slots 0 to 7 using keys `h_0`..`h_7` and `m_0`..`m_7` in NVS namespace `alarms`.
8. **`web-client`**:
   - Modern Svelte 4 SPA with Island architecture wireframe.
   - Vite configuration with `vite-plugin-compression` outputs `.gz` directly into `../data/`.

---

## 5. Build, Test, and CI Environment

- **PlatformIO Environments**:
  - `nodemcu-32s`: Target ESP32 hardware build (framework: Arduino, C++17, LittleFS filesystem, `min_spiffs.csv` partition table).
  - `google_tests`: Native desktop test runner using GoogleTest 1.15.2.
  - `unity_tests`: Native desktop test runner using Unity 2.6.1.
- **Current Test Status**:
  - `pio test -e google_tests`: **38/38 tests passing** (covers `AccelStepperMotor`, `AlarmRepository`, `ClockHand`, `SwitchGroup`, `TimeEditor`).
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
