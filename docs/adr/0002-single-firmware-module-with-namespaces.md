# Single-Firmware Module Structure with Namespaces

## Status
accepted

## Context
The firmware was previously fragmented into 9 separate PlatformIO local libraries under `lib/` (`soc-api`, `soc-esp32`, `soc-mock`, `stepper-api`, `accel-stepper-adapter`, `buttons`, `wifi-manager`, `aviator-clock`, `arduino-mock`). Many of these libraries consisted of only two to four source files. This over-modularization introduced significant operational drag:
- PlatformIO's Library Dependency Finder (LDF) suffered from compatibility scanning overhead and required brittle `lib_ignore` and `lib_deps` exclusion lists in `platformio.ini`.
- Deeply nested include directories (`lib/<name>/include/...`) complicated codebase navigation and refactoring.
- Mock libraries maintained in `lib/` conflicted with target compilation and native host unit test environments.
- None of these components were reusable outside the Aviator Clock project.

## Decision
We consolidate the firmware into a single module rooted under `src/`, organized into two primary architectural domains plus bootstrapping:
1. `aviator::domain` (`src/domain/`): Contains all domain concepts, entities, services, and ports (abstract interfaces).
2. `aviator::platform` (`src/platform/`): Contains concrete hardware and framework implementations (ESP32, FreeRTOS, LittleFS, AccelStepper).
3. `aviator::app` (`src/app/` or `src/`): Composition root and system bootstrap (`main.cpp`).

All mock implementations for unit testing move directly into the `test/` tree, eliminating mock libraries from production firmware paths.

## Consequences
- Eliminates 9 `library.json` descriptors and PlatformIO LDF scanning friction.
- Reduces namespace complexity to two main realms: `aviator::domain` (inside) and `aviator::platform` (outside).
- Dramatically simplifies `platformio.ini` configuration and speeds up build and indexing times.
- Clear, unambiguous include paths and directory structure.
- Native desktop testing targets only need to compile `src/domain/` and test-local doubles.
