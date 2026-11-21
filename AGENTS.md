# Agent Guidelines & Engineering Standards

## Language & Communication Guidelines

- **Source Code & Documentation:** Always write English in all kinds of source code, configuration files, commit messages, documentation (`README.md`, ADRs, code comments, tests, issues, specifications, `.http` files, scripts, queries, data fixtures, etc.). Never write German in any repository file, comment, or commit message.
- **User Communication:** German is allowed and used for chat/conversation with the user.

## Code Quality Guidelines

1. **No Abbreviations:** Variable, function, parameter, and class names must be written out fully to maximize readability and clarity (e.g., `calculateTotalAmount` instead of `calcTotAmt`, `stepperMotor` instead of `mot`).
2. **Clean & Expressive Code:** Code should read like prose. Identifier names must clearly express their intent and responsibility, making the code largely self-explanatory.
3. **Comments Explain HOW and WHY:** Comments must not describe *what* the code does (the code itself should be clear enough to show that). Comments explain *why* a particular design decision was made, what physical or electrical constraints apply, or *how* a non-obvious algorithm operates.

## Embedded C++ & FreeRTOS Engineering Standards

1. **Memory Discipline (Zero Dynamic Allocation in Runtime):**
   - Dynamic memory allocation (`malloc`, `free`, `new`, `delete`, resizing `std::vector`, or `std::string`) is strictly forbidden within running FreeRTOS task loops, callbacks, and Interrupt Service Routines (ISRs).
   - Dynamic allocation is only permitted once during system initialization in the Composition Root.
   - Use fixed-size storage (`std::array`, static circular buffers) and pass references or pointers.
2. **Stack Budget & Safety:**
   - FreeRTOS task stacks have finite, pre-allocated depths. Never allocate large buffers or objects on the stack. Avoid recursion.
3. **Hardware Abstraction Layer (HAL) Enforcement:**
   - Domain logic and business rules must remain strictly isolated from target hardware and platform SDKs.
   - Never directly call Arduino functions (`digitalRead`, `digitalWrite`, `delay`) or ESP-IDF APIs inside domain or application logic.
   - Always program against `lib/soc-api` interfaces (`ISoc`, `IDigitalInput`, `IDigitalOutput`, `ITask`, `ISleeper`, `IPersistentStorage`). Hardware-specific implementations belong exclusively in `lib/soc-esp32`.
4. **Concurrency & Thread Safety:**
   - Shared mutable state accessed across multiple FreeRTOS tasks must be protected using appropriate synchronization primitives (queues, mutexes, task notifications).
   - Every task loop must explicitly yield or block (`vTaskDelay`, queue wait) to prevent task starvation and watchdog timer resets.
   - Never execute blocking logic or acquire mutexes inside an ISR. Use the dedicated `*FromISR` APIs and defer processing to a task.
5. **Modern Embedded C++ Idioms:**
   - Use `const` and `constexpr` rigorously for compile-time constants instead of `#define` macros or magic numbers.
   - Use strongly typed enumerations (`enum class`).
   - Prefer value semantics, references (`const Type&`), and `std::optional` or explicit status codes over C++ exceptions or RTTI (which are disabled on embedded targets).
6. **Host Testability:**
   - Code must be designed so that all domain and business logic can be compiled and executed on native host platforms without hardware attached, utilizing `lib/soc-mock` for unit testing (GoogleTest / Unity).

## Project Context & Architecture

- **Domain Model & Glossary:** See `CONTEXT.md` at repository root. Always use this ubiquitous language when naming entities, writing specs, or filing issues.
- **System Architecture & Inventory:** See `docs/SYSTEM_INVENTORY.md` for a complete breakdown of hardware pinouts, active modules, FreeRTOS tasks, build targets, and current test statuses.

## Agent Skills

### Issue Tracker
Issues live as markdown files organized by version under `.scratch/<version>/<feature>/` (currently `.scratch/1.0/`) in this repo. See `docs/agents/issue-tracker.md`.

### Triage Labels
The triage labels are `needs-triage`, `needs-info`, `ready-for-agent`, `ready-for-human`, `wontfix`. See `docs/agents/triage-labels.md`.

### Domain Docs
Single-context layout (`CONTEXT.md` and `docs/adr/` at root). See `docs/agents/domain.md`.
