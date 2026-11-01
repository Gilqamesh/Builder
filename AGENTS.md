# Builder agent entry point

Use this file to acquire the smallest context that fully determines a task. Builder owns the shared workflow and coding instructions. Module READMEs supply the goals and semantic contracts needed to apply them across the combined workspace.

## Documentation ownership

- This file owns instruction precedence, context routing, semantic authority, scope discipline, and completion reporting.
- `docs/agent-workflow.md` owns the reasoning and decision process for semantic work.
- `docs/repository-model.md` owns intended cross-module and build-system semantics.
- [`docs/module-readme.md`](docs/module-readme.md) owns the module README convention, including the required Goal and placement of module-specific direction.
- Path-specific language instructions own concrete API, coding, and style rules.
- Public headers and module READMEs own the public contract, following the split in the [C++ documentation rules](.github/instructions/cpp.instructions.md#documentation).
- Tests provide executable evidence and case coverage.

Record each durable fact at the narrowest scope shared by everything it governs. The contract of the component named as owner is authoritative for an ownership assertion; shared architecture documentation is authoritative for relationships spanning components. Elsewhere, route readers to the authoritative source, repeating information only when correct local use requires it.

State the intended result and the criteria for achieving it. Express instructions as concrete actions.

## Instruction precedence

Resolve conflicts in this order:

1. Explicit requirements in the current task.
2. Matching path-specific files under `.github/instructions/`.
3. Repository-wide documents dispatched by this file.
4. Surrounding code conventions for matters not otherwise specified.

Repository code is authoritative for current behavior. Public headers and module READMEs are authoritative for their respective parts of the public contract. Explicit architecture and invariant documentation is authoritative for intended behavior. Report mismatches and distinguish current implementation facts, settled decisions, open decisions, and possible future directions.

## Context dispatch

For C++ work, also read:

- `.github/instructions/cpp.instructions.md`
- `.github/instructions/cpp-headers.instructions.md` when editing a `.h` file
- `.github/instructions/cpp-sources.instructions.md` when editing a `.cpp` file

Apply all matching task rows, loading only the selected documents or sections. For vendored third-party changes, use only the vendored-code row.

| Task | Additional context |
|---|---|
| Ordinary work inside one module | The module README Goal and relevant contracts; the validation section of `docs/agent-workflow.md` for implementation tasks |
| Work on public semantics, including documentation | The module README Goal, relevant model and requirements, and public headers |
| Write or review module documentation, C++ comments, or examples | `docs/module-readme.md`, the [module documentation invariant](docs/repository-model.md#module-documentation), and applicable C++ instructions for comments and C++ examples |
| Define or materially change a public API within an established module boundary | `docs/agent-workflow.md` and the module README Goal and relevant contracts |
| Create a module or materially change module boundaries | `docs/agent-workflow.md`, `docs/repository-model.md`, and `docs/module-readme.md` |
| Change workspace ordering, dependencies, artifacts, phases, bootstrap, or module execution | `docs/repository-model.md` and each affected foundation module README Goal and relevant contracts |
| Implement across multiple modules | `docs/agent-workflow.md` and each affected module README Goal and relevant contracts |
| Encounter an unresolved durable semantic choice | `docs/agent-workflow.md` and each affected module README Goal and relevant contracts |
| Review a change | Instructions for the changed paths, affected module README Goals and contracts, public interfaces and relevant README contracts of direct dependencies, and relevant validation code |
| Modify vendored third-party code | Only the task, enclosing workspace/module instructions, and the exact vendored files required |

Load only the module documentation section of `docs/repository-model.md` for documentation work; load other sections only when module boundaries or repository/build semantics are in scope.

## Context discipline

1. Resolve the exact workspace and complete module directory name.
2. Read the applicable language instructions.
3. Read the target module README Goal and relevant contracts, then its public headers when working on public semantics.
4. Read only implementation and validation files relevant to the requested behavior.
5. Read direct dependency interfaces and relevant README Goals, model, and requirements when needed; inspect their implementations only when their internals are part of the task.
6. Inspect dependents when a public contract changes or validation exposes an incompatibility.
7. Use the complete module name found in the workspace. Let the target module and, when necessary, one direct analogue supply local conventions.
8. Stop acquiring context when the requested behavior and governing constraints are understood.

## Semantic control

The user owns terminology, semantics, invariants, ownership, and architectural direction. The agent owns bounded implementation and validation.

For a new module or material public-contract change, follow `docs/agent-workflow.md`: identify missing durable decisions, ask only questions that alter those decisions, and establish the module README Goal before designing its public API. Record additional model and requirements when they help define the contract.

Keep workflow and coding instructions centralized in Builder; module READMEs describe module intent and semantics rather than a separate instruction layer. Place durable guidance with its existing documentation owner, applying `docs/module-readme.md` to module-specific direction. Keep temporary task plans, working notes, and transcripts outside permanent instruction files.

## Scope discipline

- Keep unrelated cleanup, renaming, reformatting, refactoring, and future possibilities outside the requested semantic boundary.
- Preserve established terminology; obtain direction before replacing a semantic term.
- Evaluate module reuse or extraction only in a separately requested architecture audit.

## Completion

Use the [Goal / Changes / Validation record](docs/agent-workflow.md#logical-change-record) for planning and completion. Report the intended outcome under Goal, resulting behavior and files changed under Changes, and exact checks, actual results, unresolved semantic decisions, and unverified behavior under Validation. Include the completed record in the final response.

Create a commit only when the user explicitly requests one. Completing or validating a task does not authorize a commit. When a commit is requested, include the completed record in its message.
