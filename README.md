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
- **Audio Alert System:** JQ6500-16P MP3 module connected via UART plays user-selected alarm sound tracks.
- **Single Hardware Setup Button:**
  - Short press: Dismisses an active ringing alarm.
  - 3-second long press: Enters Setup Mode (WiFi AP + Web Portal).
  - 10-second hold: Triggers factory reset (clears stored credentials and restarts).
- **Embedded Web Configuration Portal:** Embedded Single Page Application (Svelte 4 + TypeScript) served directly from LittleFS for Wi-Fi provisioning, alarm time settings, and audio tone selection with preview.
- **Modern Embedded Architecture:** Hexagonal (Ports & Adapters) design in C++17 with FreeRTOS, zero runtime dynamic allocation, and full native host testability.

---

## Hardware Architecture

### Components
- **Microcontroller:** ESP32 (NodeMCU-32S board, 240 MHz dual-core, 320 KB RAM, 4 MB Flash)
- **Actuators:** 3× Bipolar Stepper Motors (200 steps/rev, configured to 1600 effective steps/rev with 8× microstepping)
- **Motor Drivers:** TMC2209 or A4988 Step/Direction drivers with software enable control
- **Zero-Point Sensors:** 3× A3144 Unipolar Digital Hall-Effect Sensors with voltage dividers (5V to 3.3V safe)
- **Audio Output:** JQ6500-16P MP3 module connected via UART (GPIO 16 RX / GPIO 17 TX)
- **User Control:** 1× Pushbutton (GPIO 15) for Setup Mode / Factory Reset / Alarm Dismissal

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
│   ├── SetupButtonAdapter (GPIO 15)          ├── Central Motor Runner   │
│   ├── NTP Time Sync Adapter                 ├── NVS Storage Adapter    │
│   └── Web Portal (LittleFS + AsyncWeb)      ├── Audio Player (JQ6500)  │
│                                             └── System Gateway (Reset) │
│        │                                                ▲              │
│        ▼                                                │              │
│   ┌──────────────────────────────────────────────────────────┐         │
│   │               DOMAIN: aviator::domain (Pure C++17)       │         │
│   │                                                          │         │
│   │   Boundaries / Ports (Domain Interfaces):                │         │
│   │   ├── ClockInputEvents           ├── HandActuator        │         │
│   │   └── ClockConfigurationApi      ├── AlarmStorage        │         │
│   │                                  ├── TimeSource          │         │
│   │                                  ├── AudioPlayer         │         │
│   │                                  └── SystemGateway       │         │
│   │                                                          │         │
│   │   Core Domain Logic (Zero FreeRTOS / ESP32 / Arduino):   │         │
│   │   ├── DialGeometry (330° arc, angle translations)        │         │
│   │   ├── ClockEngine (Time-to-angle mapping, wrap-around)   │         │
│   │   ├── AlarmCoordinator (Real-time alarm evaluation)      │         │
│   │   ├── ClockConfigurationService (Web API coordinator)    │         │
│   │   └── OperatingStateMachine (Initial, Normal, SetupMode) │         │
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

## Local Web Client Development & Testing

You can run and test the web configuration portal entirely locally on your development machine in any modern web browser without an ESP32 connected and without flashing LittleFS.

### Starting the Local Development Server

1. **Navigate to the web client directory:**
   ```bash
   cd web-client
   ```

2. **Install dependencies:**
   ```bash
   npm install
   # or with pnpm:
   pnpm install
   ```

3. **Start the Vite development server:**
   ```bash
   npm run dev
   # or with pnpm:
   pnpm dev
   ```

4. **Open in browser:**
   Open the URL displayed in the terminal (typically `http://localhost:5173`).

Changes to files in `web-client/src/` will update instantly in the browser thanks to Vite's Hot Module Replacement (HMR).

### What Works (Mocked in Development Mode)

When running via `npm run dev`, `import.meta.env.DEV` automatically activates the built-in mock handlers in `web-client/src/lib/api.ts`:

- **Complete User Interface:** Dark/light mode theme toggle, form controls, responsive layout, and visual states.
- **Wi-Fi Scanning & Connection:** Simulates scanning available networks (`Home_WiFi`, `Guest_WiFi`, `Aviation_Club`) with realistic network delays, RSSI signal indicators, and password authentication flows.
- **Alarm Configuration:** Loading, modifying, and saving alarm time, enabled status, and sound track selection (values persist in memory for the duration of the dev session).
- **Audio Track Selection & Previews:** Displays the track list and simulates triggering/stopping audio previews (actions are logged to the browser developer console `F12`).
- **System Status Display:** Simulates system status responses (IP `192.168.4.1`, setup mode, connection flags).
- **Device Actions:** Device reboot and setup finalization trigger simulated responses and console logs.

### What Does Not Work (Hardware & ESP32 Dependencies)

Because the development server runs standalone in the browser on your PC:

- **No Physical Hardware Movement:** Stepper motors do not move; physical hands and dials do not rotate or indicate AM/PM.
- **No Physical Audio Output:** The JQ6500 MP3 module is not connected; audio preview calls do not emit sound on your PC speakers (actions are logged to the console).
- **No Real Wi-Fi Control:** The browser does not scan real physical 2.4 GHz SSIDs or change network adapter settings.
- **Non-Persistent Flash Storage:** Modified alarm settings are stored in memory and reset on page reload, unlike the persistent NVS storage on the ESP32.
- **LittleFS Gzip Delivery:** Vite serves unbundled/hot-reloaded assets from memory rather than the pre-compressed `.gz` static files delivered by the C++ firmware's LittleFS server. (To test the final bundle, run `npm run build` and flash via `pio run -t uploadfs`).

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
