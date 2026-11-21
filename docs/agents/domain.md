# Domain Docs

How engineering skills consume this repo's domain documentation when exploring the codebase.

## Before exploring, read these

- **`CONTEXT.md`** at the repo root.
- **`docs/ARCHITECTURE.md`** — target hexagonal architecture, namespaces, and FreeRTOS rules.
- **`docs/adr/`** — read ADRs that touch the area you are about to work in.

If any of these files don't exist, proceed silently. The `/domain-modeling` skill creates them lazily when terms or decisions get resolved.

## File structure

Single-context repo:

```text
/
├── CONTEXT.md
├── docs/
│   ├── adr/
│   │   └── 0001-...md
│   └── agents/
└── src/
```

## Use the glossary's vocabulary

When naming a domain concept (in an issue title, spec, refactor proposal, hypothesis, or test name), use the term as defined in `CONTEXT.md`. Do not drift to synonyms the glossary explicitly avoids.

## Flag ADR conflicts

If an output contradicts an existing ADR, surface it explicitly rather than silently overriding.
