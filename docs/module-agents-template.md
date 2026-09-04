# Module `AGENTS.md` template

A module may contain at most one agent-specific document: `AGENTS.md` at the module root.

Create it when durable, non-obvious module semantics must guide future work. Public headers, shared coding rules, dependency lists, and task-local details remain in their existing sources of truth.

For a new semantic module, draft this file before the public API. Record only established decisions and keep unresolved semantics explicit.

Keep the final file short enough to read whenever the module is touched. Omit empty sections.

```markdown
# <complete_module_name>

## Purpose

State the one responsibility this module owns and which adjacent responsibilities belong elsewhere.

## Public model

Define the user's terminology and the relationships among the public concepts. Describe ownership, lifetime, mutability, ordering, units, or representation only when they are important and not obvious from the types.

## Intended usage

Show the smallest representative public use when it materially clarifies the API or relationships among its concepts. Treat it as an API regression check: additional complexity in normal use requires a concrete semantic need.

## Invariants

- List properties that must remain true across implementations.
- Prefer observable semantics and safety properties.
- Keep formatting rules and temporary implementation choices in their existing sources of truth.

## Boundary

- State the semantic responsibilities this module owns.
- Identify tempting adjacent responsibilities that belong to another module or remain undecided.

## Validation

- Name the narrow build, test, CLI, smoke test, or manual procedure that proves the contract.
- Distinguish automated and environment-dependent validation.

## Open decisions

- Record unresolved semantic choices.
- Identify the direction required to resolve them.
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
8. Which variations are concrete current requirements?
9. What executable evidence proves correctness?

Ask only the questions whose answers are not already established and would change durable semantics.

## Maintenance rules

- Update the contract when public semantics or durable invariants change.
- Let local refactors that preserve behavior leave the contract unchanged.
- Keep task plans, implementation journals, transcripts, and historical alternatives outside this file.
- Move an item out of `Open decisions` after an explicit decision is reflected in the task or code.
- Encode invariants in types and validation where practical; shrink the document when the code becomes self-explanatory.
