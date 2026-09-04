# Builder agent entry point

Use this file to discover the minimum context required for a task. Do not recursively read the repository, every workspace, or the complete `docs/` tree.

## Instruction precedence

Resolve conflicts in this order:

1. Explicit requirements in the current task.
2. The nearest applicable module `AGENTS.md`.
3. The nearest applicable workspace `AGENTS.md`.
4. Matching path-specific files under `.github/instructions/`.
5. Repository-wide instructions and documents dispatched by this file.
6. Surrounding code conventions for matters not otherwise specified.

Repository code is authoritative for current behavior. Explicit architecture and invariant documentation is authoritative for intended behavior. When they disagree, report the mismatch and distinguish the current implementation from the intended model; do not let stale or transitional code silently determine future architecture.

## Context dispatch

For C++ work, also read:

- `.github/instructions/cpp.instructions.md`
- `.github/instructions/cpp-headers.instructions.md` when editing a `.h` file
- `.github/instructions/cpp-sources.instructions.md` when editing a `.cpp` file

Then load only the documents selected below:

| Task | Additional context |
|---|---|
| Work inside one module | That workspace's `AGENTS.md` and the module's `AGENTS.md`, when present |
| Create a module or define or materially change public semantics | `docs/agent-workflow.md`, `docs/repository-model.md`, and `docs/module-agents-template.md` when a module contract is needed |
| Change workspace ordering, dependencies, versions, artifacts, phases, bootstrap, or module execution | `docs/repository-model.md`, `ws0/AGENTS.md`, and the affected foundation module's `AGENTS.md` |
| Review a change | Instructions for the changed paths, public interfaces of direct dependencies, and relevant validation code |
| Modify vendored third-party code | Only the task, enclosing workspace/module instructions, and the exact vendored files required |

Do not load `docs/repository-model.md` for ordinary implementation work that does not affect the repository model.

## Context discipline

1. Identify the exact workspace and complete module directory name.
2. Read the target module's public headers before its implementation.
3. Read only implementation files relevant to the requested behavior.
4. Read dependency interfaces, not dependency implementations, unless the task requires their internals.
5. Read dependents only when changing a public contract or when validation exposes an incompatibility.
6. Do not scan unrelated modules for style examples; prefer the target module and one directly analogous module when necessary.
7. Do not shorten, reconstruct, or guess module names when the real directory name is available.

## Semantic control

The user owns terminology, semantics, invariants, ownership, and architectural direction. The agent owns bounded implementation and validation.

Before implementing a new module or materially changing a public contract, follow `docs/agent-workflow.md`: identify the durable decisions that are missing, ask only high-leverage questions, and draft the module's `AGENTS.md` before writing the public interface when a module contract is warranted.

When the distinction matters, label a statement as a current implementation fact, settled architectural decision, open decision, or possible future direction. Do not present one category as another.

A module may have at most one agent-specific file: `<module>/AGENTS.md`. Do not create additional plans, memory files, style files, or architecture files inside a module.

## Change discipline

- Prefer correctness and coherent semantics over rapid implementation.
- Design public semantics before private implementation mechanics. For a public API, reason from the smallest representative user-authored use first.
- Keep public APIs minimal, concise, technically correct, and easy to understand. Every public type, method, abstraction, and module must have a concrete purpose.
- Produce the smallest coherent patch that satisfies the task, then stop at the requested semantic boundary.
- Do not expand a successful change into adjacent cleanup, reformatting, renaming, reorganization, refactoring, or redesign.
- Do not let speculative future implementation strategies shape the current API. Do not add abstractions, extension points, overloads, configuration, or compatibility layers for hypothetical requirements, and do not leave empty or misleading architectural placeholders for future work.
- Preserve established terminology exactly; ask before replacing one semantic term with another.
- Do not update a module `AGENTS.md` for implementation details, temporary plans, or facts already obvious from the public interface.
- Make tests prove observable contracts, invariants, and important negative cases rather than merely execute code.
- Never claim compilation, tests, runtime behavior, or other evidence that was not actually obtained.

## Completion

Report:

1. The resulting behavior.
2. The files changed.
3. The exact validation performed and its result.
4. Any unresolved semantic decision or unverified behavior.
