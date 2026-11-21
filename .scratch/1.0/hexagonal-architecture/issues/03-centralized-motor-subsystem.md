# 03 Implement Centralized Stepper Motor Execution Subsystem

Status: ready-for-agent
Blocked by: 02

## Context
Per ADR 0003, running individual FreeRTOS tasks per hand with `sleep(1)` bottlenecks stepping speeds to at most 1000 steps/sec, causes context-switching overhead, and risks task handle corruption when destroyed during mode changes. We need a single, dedicated high-priority motor runner task that steps all 3 dials simultaneously.

## Requirements
1. Create `src/platform/motor/` in namespace `aviator::platform::motor`.
2. Migrate and adapt `AccelStepperWrapper` and `PreciseOneWayHomingStrategy` into `src/platform/motor/`.
3. Implement `StepperHandActuator` implementing domain port `aviator::domain::HandActuator`.
4. Implement `StepperSubsystem`:
   - Holds references or wrappers for the three dial actuators (`hour`, `minute`, `second`).
   - Spawns a single FreeRTOS task (`motor_runner_task`) once during initialization.
   - Runs a tight, non-blocking stepping loop calling `run()` on all active motors.
   - Accepts asynchronous position/angle updates without blocking the calling thread.
   - Guarantees zero dynamic allocation after bootstrap.
5. Create unit tests for `StepperHandActuator` and `StepperSubsystem`.

## Verification
- Run `pio test -e google_tests` to verify motor state transitions and homing logic.
- Verify compilation with `pio run -e nodemcu-32s`.

## Comments
