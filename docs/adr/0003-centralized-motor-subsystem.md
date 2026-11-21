# Centralized Stepper Motor Execution Subsystem

## Status
accepted

## Context
In the previous architecture, each `ClockHand` independently managed its own FreeRTOS task. In its internal loop, each hand task repeatedly executed `_stepperMotor->update(); sleeper.sleep(1);`. This caused multiple severe problems:
1. **Pulse Starvation & Jitter:** FreeRTOS `vTaskDelay` yields for at least one operating system tick (1 ms at 1000 Hz, or 10 ms at 100 Hz). This capped step pulse rates to at most 1000 steps/sec, leading to unacceptably slow homing, jerky movement, and inability to perform fast wrap-around jumps.
2. **Dynamic Task Lifetime Hazard:** Entering and exiting operational modes (such as Alarm Setup) destroyed and recreated FreeRTOS tasks at runtime (`vTaskDelete` followed by `xTaskCreate` with 8 KB stacks), violating zero-allocation runtime standards and creating dangling task handle risks.
3. **Scheduler Thrashing:** Running three independent stepping tasks with tight delays wastes CPU cycles in context switching.

## Decision
We replace per-hand FreeRTOS tasks with a single, centralized motor execution subsystem (`MotorSubsystem` / `MotorRunner`):
1. **One Central Stepper Runner:** A single dedicated FreeRTOS task running at high priority (or driven by a dedicated ESP32 hardware timer) drives the step updates for all three dials simultaneously without blocking delays inside the pulse generator loop.
2. **Static Task Lifecycle:** The motor runner task is spawned exactly once during system initialization in the Composition Root. It runs continuously for the lifetime of the application.
3. **Zero Dynamic Allocation at Runtime:** Target hand angles and movement commands are passed from the domain into the motor subsystem via a thread-safe message queue or atomic position targets. Hands are not stopped by killing tasks; they are instructed to hold position or step toward a new target angle.

## Consequences
- Eliminates step frequency bottlenecks, allowing smooth, high-speed movement and homing.
- Eliminates runtime heap allocations and `vTaskDelete` dangling pointer hazards.
- Drastically reduces task stack memory consumption by consolidating three separate 8 KB task stacks into a single dedicated stepping task.
