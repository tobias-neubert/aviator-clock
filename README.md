# Aviator Clock

An ESP32-powered desktop clock inspired by classic analog aircraft flight instrument dials. The Aviator Clock features three independent dials (Hours, Minutes, and Seconds) driven by precision bipolar stepper motors, calibrated with magnetic zero-point sensing, and synchronized with network time (NTP).

---

## Overview

The **Aviator Clock** recreates the aesthetics and mechanics of vintage cockpit avionics in a physical desktop timepiece. Unlike conventional clocks that move hands across a continuous 360° circle, the Aviator Clock dials feature an authentic **330.0° active arc** with an inactive gauge sector, complete with high-speed wrap-around jumps at hour and minute rollover.

### Key Features

- **Triple Dial Display:** Three dedicated dials for Hours (12h), Minutes (60m), and Seconds (60s).
- **Precision Stepper Motion:** Driven by silent stepper motor drivers (TMC2209 / A4988) with microstepping.
- **Magnetic Zero-Point Calibration (Homing):** A3144 Hall-effect sensors and neodymium magnets automatically align each hand to its zero point on power-up.
- **NTP Time Synchronization:** Automatically synchronizes system time via Wi-Fi with local timezone support.
- **Offline Fallback:** Continues accurate timekeeping even when Wi-Fi is temporarily unavailable.
- **Physical Alarm Configuration:**
  - **Slot Selector:** 3-bit binary toggle switch group to select one of 8 persistent alarm slots.
  - **Dual Rotary Encoder:** Concentric dual-axis encoder to adjust hours and minutes interactively.
  - **AM/PM Visual Indicator:** The second hand points to 12 o'clock (0°) for AM or 6 o'clock (165°) for PM during alarm adjustment.
- **Embedded Web Configuration Portal:** Embedded Single Page Application (Svelte 4 + TypeScript) served directly from LittleFS for Wi-Fi provisioning and device setup.
- **Modern Embedded Architecture:** Hexagonal (Ports & Adapters) design in C++17 with FreeRTOS, zero runtime dynamic allocation, and full native host testability.

---

## Hardware Architecture

### Components
- **Microcontroller:** ESP32 (NodeMCU-32S board, 240 MHz dual-core, 320 KB RAM, 4 MB Flash)
- **Actuators:** 3× Bipolar Stepper Motors (200 steps/rev, configured to 1600 effective steps/rev with 8× microstepping)
- **Motor Drivers:** TMC2209 or A4988 Step/Direction drivers with software enable control
- **Zero-Point Sensors:** 3× A3144 Unipolar Digital Hall-Effect Sensors with voltage dividers (5V to 3.3V safe)
- **User Controls:**
  - Dual Concentric Rotary Encoder with integrated push button
  - Mode Selection Push Button
  - 3-position DIP / Toggle Switch Group (3-bit binary slot selector)

For full electrical schematics and pinout specifications, see [`docs/SYSTEM_INVENTORY.md`](docs/SYSTEM_INVENTORY.md).

---

## Software Architecture

The firmware is designed using **Hexagonal Architecture (Ports & Adapters)** to decouple clock domain logic completely from microcontroller peripherals and operating system primitives:

```text
┌────────────────────────────────────────────────────────────────────────┐
│               PLATFORM ADAPTERS: aviator::platform                     │
│               (Outside World: Hardware, FreeRTOS, Network)             │
│                                                                        │
│   [Driving Adapters]                        [Driven Adapters]          │
│   ├── FreeRTOS Input Poller Task            ├── Central Motor Runner   │
│   ├── NTP Time Sync Adapter                 ├── NVS Storage Adapter    │
│   └── Web Portal (LittleFS + AsyncWeb)      └── WiFi / System Control  │
│        │                                                ▲              │
│        ▼                                                │              │
│   ┌──────────────────────────────────────────────────────────┐         │
│   │               DOMAIN: aviator::domain (Pure C++17)       │         │
│   │                                                          │         │
│   │   Boundaries / Ports (Domain Interfaces):                │         │
│   │   ├── ClockInputEvents           ├── HandActuator        │         │
│   │   └── ClockConfigurationApi      ├── AlarmStorage        │         │
│   │                                  ├── TimeSource          │         │
│   │                                  └── SystemGateway       │         │
│   │                                                          │         │
│   │   Core Domain Logic (Zero FreeRTOS / ESP32 / Arduino):   │         │
│   │   ├── DialGeometry (330° arc, angle translations)        │         │
│   │   ├── ClockEngine (Time-to-angle mapping, wrap-around)   │         │
│   │   ├── TimeEditor & Alarm (Slot arithmetic, AM/PM math)   │         │
│   │   └── OperatingStateMachine (Normal, AlarmSetup, WiFi)   │         │
│   └──────────────────────────────────────────────────────────┘         │
└────────────────────────────────────────────────────────────────────────┘
```

- **Clean Core:** `aviator::domain` contains zero dependencies on Arduino, FreeRTOS, or hardware headers.
- **Central Motor Runner:** A single high-priority FreeRTOS task coordinates smooth, simultaneous step execution across all three motors without individual task sleep bottlenecks.
- **Zero Runtime Allocations:** All tasks, queues, and persistent buffers are pre-allocated in the Composition Root.

For architecture details and design records, see:
- [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) — Target architecture guide
- [`docs/adr/`](docs/adr/) — Architecture Decision Records (ADRs)
- [`CONTEXT.md`](CONTEXT.md) — Ubiquitous domain terminology

---

## Getting Started

### Prerequisites
- [PlatformIO Core or PlatformIO IDE for VS Code](https://platformio.org/)
- [Node.js](https://nodejs.org/) (v18+ recommended) and `npm` (for building the Web UI)

### 1. Build the Web Client Assets
The web configuration portal is a Svelte 4 SPA that must be built into gzipped assets before flashing to the device filesystem:

```bash
cd web-client
npm install
npm run build
cd ..
```
The pre-compressed assets (`.gz`) are written directly to `data/`.

### 2. Upload the Filesystem (LittleFS)
Flash the web assets to the ESP32 LittleFS flash partition:

```bash
pio run -e nodemcu-32s -t uploadfs
```

### 3. Build & Flash Firmware
Compile the C++ firmware and flash it to the ESP32:

```bash
# Build
pio run -e nodemcu-32s

# Upload
pio run -e nodemcu-32s -t upload

# Monitor serial output (115200 baud)
pio device monitor
```

---

## Running Unit Tests

The domain core and adapters are verified using GoogleTest suites that execute natively on your development machine (Linux / macOS / Windows) without requiring physical hardware:

```bash
pio test -e google_tests
```

All 38+ unit tests run in less than 2 seconds, verifying:
- Dial geometry angle calculations and 59→0 wrap-around jumps
- Alarm slot editing, rollover wrapping, and AM/PM logic
- DIP switch binary decoding
- Motor state machine transitions and homing routines

---

## Operating Modes

1. **Normal Running Mode:** The clock advances hands in real time. The second hand ticks every second, jumping forward across the inactive dial sector on minute rollover.
2. **Alarm Setup Mode:** Hold the **Mode Button** for 3 seconds. Normal ticking pauses. Use the 3 DIP switches to select an alarm slot (0–7), turn the dual rotary encoder to adjust hours and minutes, and observe the second hand indicating AM (0°) or PM (180°). Hold the Mode Button again to save and exit.
3. **Wi-Fi Setup Mode:** Hold the **Setup Button** for 3 seconds. The clock creates an open Wi-Fi Access Point (`Aviator-Clock-Setup`). Connect with a smartphone or laptop, open your browser at `http://192.168.4.1`, scan available networks, and enter credentials.
4. **Factory Reset:** Hold the **Setup Button** for 10 seconds. The clock clears all stored Wi-Fi credentials from NVS memory and restarts.

---

## Acknowledgments

Inspired by the open-source physical design by [Instructables: ESP32 3D-Printed Aviator Clock](https://www.instructables.com/ESP32-3D-Printed-Aviator-Clock/).
