# Module `AGENTS.md` template

A module may contain at most one agent-specific document: `AGENTS.md` at the module root. Create one only when the filter below identifies direction to record.

Treat module-contract sections and questions as filters, not a checklist. Record only durable, non-obvious, module-specific direction that is not already authoritative in a more appropriate source. Use the destinations below for other material.

For a new semantic module, apply this filter before drafting the public API and keep unresolved semantics explicit.

Keep the final file short enough to read whenever the module is touched. Omit every heading that adds no useful information.

## Optional sections

After the module-name heading, include only sections selected by this catalogue:

| Section | Include only when it records |
|---|---|
| `Purpose` | The capability the module provides and the observable contract that delimits it. |
| `Public model` | Relationships among public concepts that future work must preserve and that are not already authoritative in public headers. |
| `Invariants` | Durable module-specific observable or safety properties that are not already authoritative in the public contract. |
| `Validation` | A non-obvious module-specific entry point, environment, or manual requirement needed to prove the contract. |
| `Open decisions` | Unresolved semantic choices and the direction required to settle them. |

Use these destinations for material excluded by the filter:

| Material | Destination |
|---|---|
| Public contracts and API summaries | Public headers. |
| Shared coding and style rules | Matching files under `.github/instructions/`. |
| Dependency information | The source graph, as defined by [Dependency discovery](repository-model.md#dependency-discovery). |
| Implementation rationale and private storage maintenance | Beside the code they govern. |
| Executable evidence and test-case inventories | Tests. |
| Routine build/test commands and obtained results | The task's validation report, using existing build/test entry points. |
| Task plans, implementation journals, transcripts, historical alternatives, temporary choices, and task-local details | Task context outside the module contract; keep temporary material outside permanent instruction files. |

## Optional questions for the first draft

Use a question only when the task, module, or a current dependency raises it and the answer is not already authoritative elsewhere:

1. What capability does the module provide, and which inputs, outputs, or invariants delimit it?
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
- Move an item out of `Open decisions` after an explicit decision is reflected in the task or code.
- Shrink the document when another authoritative source makes its direction redundant.
