# Hexagonal Architecture for Core Clock Logic and Hardware Decoupling

## Status
accepted

## Context
The application logic (clock dial mathematics, wrap-around jump handling, operational mode state machines, and alarm editing) was previously entangled with hardware drivers, FreeRTOS tasks, and low-level step generation. The existing `ISoc` hardware abstraction layer served as a monolithic God Object, forcing domain objects like `ClockHand` to coordinate their own FreeRTOS tasks, busy-wait on motors, and directly invoke operating system sleeps. This tightly coupled clock business logic to target hardware and made unit testing on the host platform dependent on complex, cascading mock objects.

## Decision
We adopt a simple Hexagonal Architecture (Ports & Adapters) for the Aviator Clock firmware:
1. **Domain (`aviator::domain`):** Contains the entire inside of the hexagon — both the domain rules/services (`ClockEngine`, `DialGeometry`, `TimeEditor`, `Alarm`, `ClockStateMachine`) and the abstract Ports (`HandActuator`, `AlarmStorage`, `TimeSource`, `ClockInputEvents`, `SystemGateway`). Interface names do not use an 'I' prefix. The domain code contains zero platform headers, zero FreeRTOS references, and zero hardware knowledge. Ports belong directly to the domain because the domain defines the contracts it requires from the external world (Dependency Inversion Principle).
2. **Platform Adapters (`aviator::platform`):** Contains the outside of the hexagon, implementing or interacting with domain ports using ESP32, FreeRTOS, LittleFS, and peripheral drivers (`StepperSubsystem`, `StepperHandActuator`, `InputPollerTask`, `NvsAlarmStorage`, `WebPortalServer`).
3. **Application Bootstrapping (`aviator::app`):** Contains the Composition Root that instantiates platform adapters and injects them into the domain.

## Consequences
- The domain core and its port definitions live together in `aviator::domain`, maximizing cohesion and avoiding artificial namespace boundaries.
- The domain can be compiled natively and verified in microseconds with GoogleTest without any hardware mocks.
- Actuator mechanics (stepper motor stepping, homing strategies, driver ICs) can change or be replaced without altering clock rules.
- Eliminates the monolithic `ISoc` facade in favor of specific, narrow ports conforming to the Interface Segregation Principle.
