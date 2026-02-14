# Project Description: Aviator Clock

This document serves as the central context for the Gemini assistant to understand the goals, architecture, and vision of the project.

**Assistant Instruction:** Read this file at the beginning of every interaction to load the full project context.

## 1. Vision

The goal is to build a physical clock inspired by classic analog flight instruments. The clock will feature three separate hands for hours, minutes, and seconds. Each of these hands is designed like a separate instrument and is driven by a dedicated stepper motor.

## 2. Architecture Goals

*   **Modularity:** A clean, layered architecture that decouples application logic from hardware.
*   **Testability:** Application logic should be testable on a host PC (e.g., using GoogleTest) as much as possible without requiring physical hardware (ESP32). This is achieved through a hardware abstraction layer.
*   **Real-time Capabilities:** The system should run on a real-time operating system (RTOS, specifically FreeRTOS on the ESP32) to ensure precise and concurrent control of the three motors as well as other services (e.g., NTP time synchronization).

## 3. Software Architecture (Target State)

The software architecture follows the "Composition Root" pattern to ensure a clean separation of concerns and maximize testability.

*   **`main.cpp` (Entry Point / "Main"):**
    *   The `main.cpp` serves exclusively as the application's entry point.
    *   Its only responsibility is to instantiate the global `soc` abstraction layer (e.g., `ESP32Soc`) and the main application class `AviatorClock` and manage their lifecycle (`setup()` and `start()`).
    *   It contains no application or wiring logic whatsoever.

*   **`AviatorClock` Class (The Application / "Composition Root"):**
    *   This class is the heart of the application and resides in the `aviator-clock` library.
    *   It acts as the "Composition Root": It is responsible for creating, configuring, and connecting all objects and components necessary for operation.
    *   This includes motors, sensors, buttons, homing strategies, and the individual clock hands (`ClockHand` instances).
    *   All static configurations (pin definitions, speeds, etc.) are encapsulated within this class.
    *   It orchestrates high-level logic, e.g., "Check at startup if the setup button is pressed. If yes, start WiFi configuration mode. If no, start normal clock operation."

*   **Libraries (in `lib/`):**
    *   The individual libraries (`soc-api`, `stepper-api`, `buttons`, etc.) are reusable, independent modules with clearly defined interfaces (APIs).
    *   The `AviatorClock` class uses these libraries, but the libraries themselves have no knowledge of the `AviatorClock` application.

## 4. Core Components (Libraries in `lib/`)

*   `soc-api`: Defines the interfaces for hardware abstraction (System-on-a-Chip API). This includes services like logging, timers, GPIOs, and in the future, RTOS primitives (tasks, queues).
*   `soc-esp32`: The concrete implementation of the `soc-api` for the ESP32 platform using the Arduino/ESP-IDF Framework.
*   `soc-mock`: A mock implementation of the `soc-api` for the host test environment. It simulates hardware behavior to enable tests without a real chip.
*   `stepper-api`: Defines interfaces for controlling stepper motors (`IStepperMotor`, `IHomingStrategy`, etc.).
*   `accel-stepper-adapter`: An adapter library that encapsulates the `AccelStepper` third-party library and makes it compatible with the `stepper-api`.
*   `aviator-clock`: The actual application logic. It contains the `ClockHand` classes and the logic to map time to motor positions.

## 4. Current Status

The project is currently migrating from a single-loop architecture to an RTOS-based architecture. The migration process is documented in `MIGRATIONSPLAN.md`.

## 5. Technical Documentation

Detailed technical specifications, concepts, and diagrams (such as pinouts or homing strategies) are stored in the [`doc/specifications`](./doc/specifications) directory.

## 6. Code Quality Guidelines

1.  **No Abbreviations:** Variable, function, and class names should be written out fully to maximize readability (e.g., `calculateTotalAmount` instead of `calcTotAmt`).
2.  **Clean Code / Expressive Code:** Code should be written to read like prose. Identifier names should clearly express their intent and function, making the code largely self-explanatory.
3.  **Comments Explain the HOW and WHY:** Comments should not describe *what* the code does (the code itself should do that), but *why* a certain decision was made or *how* a complex algorithm works in detail.