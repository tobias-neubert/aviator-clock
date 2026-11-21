# Refactoring Specification: Migration to Hexagonal Architecture & Single Module

## 1. Objective
Refactor the Aviator Clock codebase from its current over-fragmented structure (legacy PlatformIO libraries in `lib/`, monolithic `ISoc` God Object, runtime task re-creation) to the clean Hexagonal Architecture defined in `docs/ARCHITECTURE.md` and ADRs 0001–0004, tailored for the simplified **Version 1.0 scope**.

## 2. Simplified Version 1.0 Requirements & Hardware Baseline
1. **Three Stepper Motors & Dial Indicators:**
   - Hours, Minutes, and Seconds dials (active arc 330.0°).
   - Each dial driven by a bipolar stepper motor via step/dir driver.
   - Zero-point sensing via 3 unipolar Hall sensors (A3144).
2. **Single Hardware Control Button:**
   - One Setup Button (`PIN_SETUP_BUTTON`):
     - 3-second long press: Enters Setup Mode (WiFi AP + Web Portal).
     - 10-second long press: Triggers Factory Reset (erases NVS credentials and restarts).
     - Short press: Dismisses active ringing alarm.
3. **Sound Module (JQ6500-16P):**
   - 1x JQ6500-16P MP3 module connected via UART to play alarm sound tracks.
4. **Web-Based Management:**
   - All configuration is performed exclusively via the embedded Web Portal (Svelte 4 SPA):
     - Wi-Fi network selection, scanning, and credentials.
     - Alarm configuration: time (hour/minute), alarm toggle (on/off), and sound track selection with test preview.
   - No physical rotary encoders, no physical DIP switches / slot selectors, and no physical mode switch buttons.

## 3. Guiding Architectural Principles
1. **Hexagonal Isolation:** The domain lives in `aviator::domain` (`src/domain/`). Pure C++17 entities, business rules, dial geometry, alarm evaluation, and abstract ports. Zero FreeRTOS, ESP32, or Arduino dependencies.
2. **No 'I' Prefix:** Interface names do not start with 'I' (e.g. `HandActuator`, `AlarmStorage`, `TimeSource`, `AudioPlayer`, `ClockInputEvents`, `ClockConfigurationApi`, `SystemGateway`).
3. **Single Firmware Module:** All production code lives under `src/` (`domain/`, `platform/`, `app/`). The legacy libraries in `lib/` are eliminated.
4. **Centralized Stepper Execution:** A single high-priority FreeRTOS task steps all 3 dials simultaneously without blocking delays inside the pulse generation loop.
5. **Zero Runtime Dynamic Allocation:** All tasks, queues, and persistent buffers are initialized once during startup in `CompositionRoot`.
6. **Incremental Verifiability:** Each refactoring step leaves the repository compiling and all native unit tests passing.

## 4. Incremental Execution Roadmap

```text
Phase 1: Build Baseline
  Ticket 01: [DONE] Fix ESP32 Build Baseline, Debug Flags & Clean Obsolete Encoders

Phase 2: Domain Core & Ports
  Ticket 02: [DONE] Extract Pure Domain Core & Abstract Ports into src/domain/

Phase 3: Platform Subsystems & Adapters
  Ticket 03: Implement Centralized Stepper Motor Execution Subsystem (3 Hands & Homing)
  Ticket 04: Implement Platform Storage (NVS) & Time (SNTP) Adapters
  Ticket 05: Implement Platform Input Adapter (Single Setup Button)
  Ticket 06: Implement Platform Web Portal, Audio Adapters (JQ6500-16P) & Web UI

Phase 4: Composition Root & Legacy Deletion
  Ticket 07: Assemble CompositionRoot, Migrate main.cpp, and Remove Legacy lib/
```

## 5. Definition of Done
- `pio run -e nodemcu-32s` builds cleanly with zero warnings or errors.
- `pio test -e google_tests` runs and passes 100% on the native host.
- The `lib/` directory is completely removed or contains zero custom firmware libraries.
- Web client supports Wi-Fi setup and alarm/sound configuration.
- `docs/ARCHITECTURE.md` and `docs/SYSTEM_INVENTORY.md` perfectly match the actual codebase.
