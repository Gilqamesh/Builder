# Module `AGENTS.md` template

A module may contain at most one agent-specific document: `AGENTS.md` at the module root.

Create it when a module has durable, non-obvious semantics that should constrain future work. Do not create it merely to repeat the public header, coding style, dependency list, or current task.

For a new semantic module, draft this file before the public API. Keep unresolved decisions explicit rather than filling them with plausible guesses.

Keep the final file short enough to read whenever the module is touched. Omit empty sections.

```markdown
# <complete_module_name>

## Purpose

State the one responsibility this module owns and the boundary beyond which another module should take over.

## Public model

Define the user's terminology and the relationships among the public concepts. Describe ownership, lifetime, mutability, ordering, units, or representation only when they are important and not obvious from the types.

## Intended usage

Show the smallest representative public use only when it materially clarifies the API or the relationship among its concepts. Treat it as an API regression check: additional complexity in normal use requires semantic justification. Do not turn this file into an API tutorial.

## Invariants

- List properties that must remain true across implementations.
- Prefer observable semantics and safety properties.
- Do not list formatting rules or temporary implementation choices.

## Non-goals

- State tempting adjacent features or abstractions that this module intentionally does not own.
- Use this section to prevent speculative expansion.

## Validation

- Name the narrow build, test, CLI, smoke test, or manual procedure that proves the contract.
- Distinguish automated and environment-dependent validation.

## Open decisions

- Record unresolved semantic choices.
- State that an agent must obtain explicit direction before resolving them.
```

## Questions for the first draft

Ask only questions that determine stable semantics:

1. What exact abstraction does the module represent?
2. What is intentionally outside the module?
3. Who owns each resource, and what must outlive what?
4. What states are valid, invalid, disconnected, empty, or incomplete?
5. What ordering, units, coordinate system, representation, or identity is observable?
6. How are failures reported and recovered from?
7. Is the module thread-safe, thread-confined, or single-threaded?
8. Which future variations are concrete requirements now, and which are only possibilities?
9. What executable evidence proves correctness?

Do not ask all questions mechanically. Ask only those the current task and code do not already answer.

## Maintenance rules

- Update the contract when public semantics or durable invariants change.
- Do not update it for local refactors that preserve behavior.
- Do not store task plans, implementation journals, transcripts, or historical alternatives here.
- Move an item out of `Open decisions` only after an explicit decision is reflected in the code or task.
- Prefer encoding invariants in types and validation; shrink the document when the code becomes self-explanatory.
