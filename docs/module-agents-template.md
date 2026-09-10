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

Use the [semantic decision questions](agent-workflow.md#settle-semantic-decisions) for unresolved direction that passes the module-contract filter above.

## Maintenance rules

- Update the contract when its durable module-specific direction changes.
- Let local refactors that preserve behavior leave the contract unchanged.
- Move an item out of `Open decisions` after an explicit decision is reflected in the task or code.
- Shrink the document when another authoritative source makes its direction redundant.
