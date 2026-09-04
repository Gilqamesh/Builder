# Module `AGENTS.md` template

A module may contain at most one agent-specific document: `AGENTS.md` at the module root. Create one only when the filter below identifies direction to record.

Treat module-contract sections and questions as filters, not a checklist. Record only durable, non-obvious, module-specific direction that is not already authoritative in a more appropriate source. Public headers, shared coding rules, dependency lists, and task-local details remain in their existing sources of truth. Tests retain executable evidence and case coverage.

For a new semantic module, apply this filter before drafting the public API and keep unresolved semantics explicit.

Keep the final file short enough to read whenever the module is touched. Omit every heading that adds no useful information.

```markdown
# <complete_module_name>

## Purpose

State the module's current responsibility and positively assign current adjacent responsibilities to their owning modules.

## Public model

Record only relationships among public concepts that future work must preserve and that are not already authoritative in the public contract.

## Invariants

- Record durable module-specific properties that must remain true across implementations and are not already authoritative in the public contract.
- Prefer observable semantics and safety properties.
- Keep private storage maintenance, formatting rules, and temporary implementation choices in their existing sources of truth.

## Validation

- Identify the narrow build, test, CLI, smoke test, or manual entry point that proves the contract.
- Add environmental or manual requirements only when they are durable and materially relevant. Detailed case coverage belongs in tests.

## Open decisions

- Record unresolved semantic choices.
- Identify the direction required to resolve them.
```

## Optional questions for the first draft

Use a question only when the task, module, or a current dependency raises it and the answer is not already authoritative elsewhere:

1. What current responsibility does the module own, and which modules own adjacent current responsibilities?
2. Which terminology or relationships among concepts require durable direction?
3. Does ownership, lifetime, or mutability affect public use?
4. Which valid, invalid, disconnected, empty, or incomplete states require a durable decision?
5. Which observable ordering, units, coordinate system, representation, or identity is not clear from the public contract?
6. Which current failures require defined reporting or recovery?
7. Which variations are concrete current requirements?
8. What executable evidence proves correctness?

## Maintenance rules

- Update the contract when its durable module-specific direction changes.
- Let local refactors that preserve behavior leave the contract unchanged.
- Keep task plans, implementation journals, transcripts, and historical alternatives outside this file.
- Move an item out of `Open decisions` after an explicit decision is reflected in the task or code.
- Shrink the document when another authoritative source makes its direction redundant.
