# Architecture Guide: Aviator Clock

This document defines the target architecture, structural layout, concurrency patterns, and engineering standards for the Aviator Clock firmware. All agents and developers modifying the codebase must conform to the principles established here.

---

## 1. Architectural Style: Hexagonal (Ports & Adapters)

The firmware is structured using **Hexagonal Architecture** (also known as Ports & Adapters).

The system consists of two primary architectural realms, unified by the Composition Root:

```text
┌─────────────────────────────────────────────────────────────────────────────────┐
│                    PLATFORM ADAPTERS: aviator::platform                         │
│                    (Outside World: Hardware, FreeRTOS, Network)                 │
│                                                                                 │
│   [Driving / Inbound Adapters]                      [Driven / Outbound]         │
│   ├── FreeRTOS Input Poller Task                    ├── Central Motor Runner    │
│   │   (GPIO, Encoders, DIP switches)                │   (AccelStepper / Pulse)  │
│   ├── NTP Sync Adapter                              ├── NVS Storage Adapter     │
│   │   (ESP-IDF SNTP)                                │   (ESP32 Preferences)     │
│   └── Web Portal Adapter                            ├── WiFi Adapter            │
│       (ESPAsyncWebServer + LittleFS)                └── System Control Adapter  │
│                                                         (ESP32 reset / LEDs)    │
│        │                                                        ▲               │
│        │ calls                                                  │ implements    │
│        ▼                                                        │               │
│   ┌─────────────────────────────────────────────────────────────────────────┐   │
│   │                 DOMAIN: aviator::domain (Pure C++17)                    │   │
│   │                                                                         │   │
│   │   Boundaries / Ports (Abstract interfaces owned by the domain):         │   │
│   │   ├── ClockInputEvents           ├── HandActuator                       │   │
│   │   └── ClockConfigurationApi      ├── AlarmStorage                       │   │
│   │                                  ├── TimeSource                         │   │
│   │                                  └── SystemGateway                      │   │
│   │                                                                         │   │
│   │   Core Domain Logic & Entities (Zero FreeRTOS / ESP32 / Arduino):       │   │
│   │   ├── DialGeometry (330° arc, degree/step translations)                 │   │
│   │   ├── ClockEngine (Translates time to dial angles, wrap-around jump)    │   │
│   │   ├── TimeEditor & Alarm (Slot arithmetic, AM/PM calculation)           │   │
│   │   └── OperatingStateMachine (NormalRunning, AlarmSetup, WifiSetup)      │   │
│   └─────────────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────────────┘
                                         ▲
                                         │ instantiates & wires together
                         ┌───────────────────────────────┐
                         │       APP: aviator::app       │
                         │     CompositionRoot & main    │
                         └───────────────────────────────┘
```

### Relevant Architecture Decision Records (ADRs)
- [ADR 0001: Hexagonal Architecture for Core Clock Logic and Hardware Decoupling](adr/0001-hexagonal-architecture.md)
- [ADR 0002: Single-Firmware Module Structure with Namespaces](adr/0002-single-firmware-module-with-namespaces.md)
- [ADR 0003: Centralized Stepper Motor Execution Subsystem](adr/0003-centralized-motor-subsystem.md)
- [ADR 0004: Web Client Isolation and Asset Distribution Pipeline](adr/0004-web-client-and-asset-pipeline.md)

---

## 2. Directory Structure and Namespaces

Rather than splitting the firmware into multiple isolated PlatformIO libraries under `lib/`, the code is organized into a single cohesive module under `src/` partitioned cleanly by C++ namespaces:

```text
aviator-clock/
├── src/
│   ├── domain/                      # Namespace: aviator::domain (Pure C++17)
│   │   ├── DialGeometry.h/.cpp      # Dial angle calculations for 330.0° active arc
│   │   ├── ClockEngine.h/.cpp       # Translates time into dial hand targets
│   │   ├── TimeEditor.h/.cpp        # Alarm editing logic and unit wrapping
│   │   ├── Alarm.h                  # Plain data structures for alarm slots
│   │   ├── OperatingStateMachine.h  # State machine for operating modes
│   │   │
│   │   │   # Ports (Domain boundary interfaces — NO 'I' prefix)
│   │   ├── HandActuator.h           # Actuator interface (setTargetAngle, home, isHomed)
│   │   ├── AlarmStorage.h           # Persistent storage contract for alarm slots
│   │   ├── TimeSource.h             # Time provider interface (current UTC/local time)
│   │   ├── ClockInputEvents.h       # Inbound user interaction events
│   │   ├── ClockConfigurationApi.h  # Inbound clock management interface
│   │   └── SystemGateway.h          # System operations (restart, factory reset, status)
│   │
│   ├── platform/                    # Namespace: aviator::platform (ESP32 & FreeRTOS)
│   │   ├── board_support_package/
│   │   │   └── PinConfig.h          # Hardware pin assignments and electrical parameters
│   │   ├── motor/
│   │   │   ├── StepperSubsystem.h   # Central FreeRTOS motor execution task
│   │   │   └── StepperHandActuator.h# Implementation of HandActuator
│   │   ├── input/
│   │   │   ├── InputPollerTask.h    # Task polling buttons, DIP switches, encoders
│   │   │   └── RotaryEncoder.h      # Quadrature rotary encoder driver
│   │   ├── storage/
│   │   │   └── NvsAlarmStorage.h    # Implementation of AlarmStorage via Preferences
│   │   ├── time/
│   │   │   └── NtpTimeSource.h      # Implementation of TimeSource via ESP-IDF SNTP
│   │   └── web/
│   │       └── WebPortalServer.h    # AsyncWebServer + LittleFS configuration portal
│   │
│   ├── app/                         # Namespace: aviator::app
│   │   └── CompositionRoot.h/.cpp   # Dependency injection, object lifecycle wiring
│   └── main.cpp                     # Arduino bootstrap: calls CompositionRoot, deletes loop
│
├── web-client/                      # Svelte 4 + TypeScript + Vite web application
├── data/                            # LittleFS filesystem binary assets (gzipped HTML/JS/CSS)
├── test/
│   ├── gtests/                      # GoogleTest unit tests (compiled natively for host)
│   └── mocks/                       # Native host test doubles for domain ports
└── platformio.ini                   # PlatformIO configuration
```

---

## 3. Layer Separation & Domain Purity

### The Domain (`aviator::domain`)
- **Strict Isolation:** Code in `src/domain/` must **never** include `<Arduino.h>`, `<freertos/...>`, `<WiFi.h>`, or any hardware headers.
- **Ports Belong to the Domain:** Ports (`HandActuator`, `AlarmStorage`, `TimeSource`, `ClockInputEvents`, `SystemGateway`) are abstract interfaces defined directly within `aviator::domain` because the domain defines the contracts it requires to operate.
- **Naming Rule — No 'I' Prefix:** Do not prefix abstract interface classes with 'I' (e.g. use `HandActuator`, not `IHandActuator`; `AlarmStorage`, not `IAlarmStorage`). Interfaces in C++ are standard abstract base classes.
- **Host Testability:** The entire domain must compile on Linux/macOS/Windows under standard `g++` / `clang++` without special toolchains.
- **Clock Dial Rules:** 
  - The dial has an active arc of `330.0°`.
  - The gap between 59 and 0 (or 11 and 0 for hours) requires forward clockwise wrap-around jumping handled deterministically in the domain math.
  - Second hand AM/PM indicator: 0° represents AM, 165° (or 30 on second scale) represents PM during Alarm Setup.
- **Type Safety:** Value semantics, references, status codes, and `std::optional` are preferred over throwing C++ exceptions.

---

## 4. FreeRTOS & Concurrency Standards

### Zero Dynamic Allocation at Runtime
- All FreeRTOS tasks, static queues, mutexes, and major buffers are instantiated **once** during startup in `CompositionRoot`.
- Tasks run continuously for the system lifetime. **Never call `vTaskDelete()` on running application tasks during mode changes.**
- Suspending or activating subsystem behavior is performed using FreeRTOS Task Notifications (`xTaskNotifyWait`, `ulTaskNotifyTake`) or Event Groups.

### Centralized Motor Execution Subsystem
- Stepper motor step generation requires sub-millisecond precision.
- **Forbidden:** Calling `vTaskDelay(1)` or `sleep(1)` inside individual motor stepping loops.
- **Required:** A single dedicated high-priority FreeRTOS task (`StepperSubsystem`) periodically processes all active motors (`AccelStepper::run()`) or utilizes ESP32 hardware timers/RMT.
- Dial target angles are dispatched asynchronously to the motor subsystem without blocking the caller.

### Safe Input Handling
- Physical switches and rotary encoders are polled and debounced by a dedicated low-priority input task (`InputPollerTask`) running at a fixed period (e.g. 20 ms).
- When a user interaction occurs (e.g., long press, slot change, encoder rotation), the input adapter translates the raw electrical event into a strongly-typed domain event call on `ClockInputEvents`.
- Polling loops must never block on motor motion or network I/O.

---

## 5. Web Client & Asset Pipeline

- Source files reside exclusively in `web-client/`.
- Build tooling (Vite) compiles and gzips assets directly into the firmware flash directory `data/`:
  - `data/index.html.gz`
  - `data/assets/*.js.gz`
  - `data/assets/*.css.gz`
- The firmware serves these assets with `Content-Encoding: gzip` directly from LittleFS flash storage.
- All HTTP REST API endpoints (`/api/...`) interact only with domain ports (`ClockConfigurationApi`, `SystemGateway`), keeping the web server purely an external adapter.
