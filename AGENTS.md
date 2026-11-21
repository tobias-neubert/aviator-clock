# Agent Guidelines & Engineering Standards

## Language & Communication Guidelines

- **Source Code & Documentation:** Always write English in all kinds of source code, configuration files, commit messages, documentation (`README.md`, ADRs, code comments, tests, issues, specifications, `.http` files, scripts, queries, data fixtures, etc.). Never write German in any repository file, comment, or commit message.
- **User Communication:** German is allowed and used for chat/conversation with the user.

## Code Quality Guidelines

1. **No Abbreviations:** Variable, function, parameter, and class names must be written out fully to maximize readability and clarity (e.g., `calculateTotalAmount` instead of `calcTotAmt`, `stepperMotor` instead of `mot`).
2. **Clean & Expressive Code:** Code should read like prose. Identifier names must clearly express their intent and responsibility, making the code largely self-explanatory.
3. **Comments Explain HOW and WHY:** Comments must not describe *what* the code does (the code itself should be clear enough to show that). Comments explain *why* a particular design decision was made, what physical or electrical constraints apply, or *how* a non-obvious algorithm operates.

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
