# Refactoring Specification: Migration to Hexagonal Architecture & Single Module

## 1. Objective
Refactor the Aviator Clock codebase from its current over-fragmented, tightly coupled structure (9 local PlatformIO libraries, monolithic `ISoc` God Object, per-hand FreeRTOS tasks with `sleep(1)` delays, runtime task re-creation) to the clean Hexagonal Architecture defined in `docs/ARCHITECTURE.md` and ADRs 0001–0004.

## 2. Guiding Architectural Principles
1. **Hexagonal Isolation:** The domain lives in `aviator::domain` (`src/domain/`). It contains pure C++17 entities, business rules, dial geometry, and abstract ports. Zero FreeRTOS, ESP32, or Arduino dependencies.
2. **No 'I' Prefix:** Interface names do not start with 'I' (e.g. `HandActuator`, `AlarmStorage`, `TimeSource`, `ClockInputEvents`, `ClockConfigurationApi`, `SystemGateway`).
3. **Single Firmware Module:** All production code lives under `src/` (`domain/`, `platform/`, `app/`). The 9 legacy libraries in `lib/` are eliminated.
4. **Centralized Stepper Execution:** A single high-priority FreeRTOS task or timer drives all 3 stepper motors without blocking delays inside the pulse generation loop.
5. **Zero Runtime Dynamic Allocation:** All tasks, queues, and persistent buffers are initialized once during startup in `CompositionRoot`. No `vTaskDelete` or dynamic task creation on mode transitions.
6. **Incremental Verifiability:** Each refactoring step must leave the repository in a compiling state with all native unit tests passing.

## 3. Incremental Execution Roadmap

```text
Phase 1: Build Baseline
  Ticket 01: Fix ESP32 Build Baseline & Replace Deprecated ESP32Encoder

Phase 2: Domain Core & Ports
  Ticket 02: Extract Pure Domain Core & Abstract Ports into src/domain/

Phase 3: Platform Subsystems & Adapters
  Ticket 03: Implement Centralized Stepper Motor Execution Subsystem
  Ticket 04: Implement Platform Storage (NVS) & Time (SNTP) Adapters
  Ticket 05: Implement Platform Input Adapter (Buttons, Encoders, DIP Switches)
  Ticket 06: Implement Platform Web Portal Adapter (LittleFS, AsyncWebServer)

Phase 4: Composition Root & Legacy Deletion
  Ticket 07: Assemble CompositionRoot, Migrate main.cpp, and Remove lib/
```

## 4. Definition of Done
- `pio run -e nodemcu-32s` builds cleanly with zero warnings or errors.
- `pio test -e google_tests` runs and passes 100% on the native host.
- The `lib/` directory is completely removed or contains zero custom firmware libraries.
- `docs/ARCHITECTURE.md` perfectly matches the actual codebase.
