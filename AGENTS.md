# Builder agent entry point

Use this file to acquire the smallest context that fully determines a task. Builder owns the repository-wide agent documentation; other repositories may provide module-local `AGENTS.md` contracts only.

## Documentation ownership

- This file owns instruction precedence, context routing, semantic authority, scope discipline, and completion reporting.
- `docs/agent-workflow.md` owns the reasoning and decision process for semantic work.
- `docs/module-agents-template.md` owns the filter and structure for module-specific direction.
- Path-specific language instructions own concrete API, coding, and style rules.
- Public headers own the public contract.
- Tests provide executable evidence and case coverage.

Record each durable fact in the source that owns it, at the narrowest scope shared by everything it governs. Elsewhere, route readers to that source instead of restating its contract. Repeat information only when correct local use requires it, and do not let a summary become a competing source of truth.

## Instruction precedence

Resolve conflicts in this order:

1. Explicit requirements in the current task.
2. The nearest applicable module `AGENTS.md`.
3. Matching path-specific files under `.github/instructions/`.
4. Repository-wide documents dispatched by this file.
5. Surrounding code conventions for matters not otherwise specified.

Repository code is authoritative for current behavior, and public headers are authoritative for the public contract. Explicit architecture and invariant documentation is authoritative for intended behavior. Report mismatches and distinguish current implementation facts, settled decisions, open decisions, and possible future directions.

## Context dispatch

For C++ work, also read:

- `.github/instructions/cpp.instructions.md`
- `.github/instructions/cpp-headers.instructions.md` when editing a `.h` file
- `.github/instructions/cpp-sources.instructions.md` when editing a `.cpp` file

Then load only the documents selected below:

| Task | Additional context |
|---|---|
| Ordinary work inside one module | The module's `AGENTS.md`, when present |
| Define or materially change a public API within an established module boundary | `docs/agent-workflow.md` and the module's `AGENTS.md`, when present |
| Create a module or materially change module boundaries | `docs/agent-workflow.md`, `docs/repository-model.md`, and `docs/module-agents-template.md` when a module contract is warranted |
| Change workspace ordering, dependencies, artifacts, phases, bootstrap, or module execution | `docs/repository-model.md` and each affected foundation module's `AGENTS.md`, when present |
| Implement across multiple modules | The multi-module dispatch section of `docs/agent-workflow.md` and each affected module's `AGENTS.md`, when present |
| Review a change | Instructions for the changed paths, public interfaces of direct dependencies, and relevant validation code |
| Modify vendored third-party code | Only the task, enclosing workspace/module instructions, and the exact vendored files required |

Load `docs/repository-model.md` only when module boundaries or repository/build semantics are in scope.

## Context discipline

1. Resolve the exact workspace and complete module directory name.
2. Read the applicable language instructions.
3. Read the target module's `AGENTS.md` when present, then its public headers.
4. Read only implementation and validation files relevant to the requested behavior.
5. Read direct dependency interfaces when needed; inspect their implementations only when their internals are part of the task.
6. Inspect dependents when a public contract changes or validation exposes an incompatibility.
7. Use the complete module name found in the workspace. Let the target module and, when necessary, one direct analogue supply local conventions.
8. Stop acquiring context when the requested behavior and governing constraints are understood.

## Semantic control

The user owns terminology, semantics, invariants, ownership, and architectural direction. The agent owns bounded implementation and validation.

For a new module or material public-contract change, follow `docs/agent-workflow.md`: identify missing durable decisions, ask only questions that alter those decisions, and draft the module contract before its public interface when a contract is warranted.

A module may have at most one agent-specific file: `<module>/AGENTS.md`. Keep plans, memory, style, and architecture guidance in the existing repository-owned documents rather than adding module-local layers.

## Scope discipline

- Keep unrelated cleanup, renaming, reformatting, refactoring, and future possibilities outside the requested semantic boundary.
- Preserve established terminology; obtain direction before replacing a semantic term.

## Completion

Report:

1. The resulting behavior.
2. The files changed.
3. The exact validation performed and its result.
4. Any unresolved semantic decision or unverified behavior.
