# Agent workflow

This workflow converts user intent into a small, reviewable change while keeping durable module semantics explicit.

## Classify the task

Use one of these modes before acquiring context:

- **Investigate or review:** inspect and report; do not edit unless requested.
- **Local implementation:** preserve the existing module contract and implement the requested behavior.
- **Semantic or public API change:** identify the contract decisions being changed before editing code.
- **New module:** define the module contract before defining its public interface.

Do not turn a local implementation task into an architecture redesign.

## Acquire context

1. Resolve the exact target module from its path or complete module name.
2. Follow the dispatch rules in `/AGENTS.md`.
3. Read the target module's public headers.
4. Read its `AGENTS.md`, when present.
5. Read only the implementation and validation paths relevant to the task.
6. Read direct dependency interfaces only when the target code uses them.
7. Stop acquiring context once the requested behavior and constraints are understood.

Context collection is not progress by itself. Avoid broad repository searches when the module boundary already identifies the relevant code.

## Review a public abstraction

For a new or materially changing public abstraction:

1. Identify its responsibility and semantic boundary.
2. Inspect the current implementation and existing dependency interfaces.
3. Write or inspect the smallest representative user-facing use.
4. Settle terminology, ownership, mutability, invariants, and observable behavior.
5. Identify explicit non-goals.
6. Decide whether an existing module owns part of the problem.
7. Decide whether a new module or abstraction is warranted under `docs/repository-model.md`.
8. Only after the public model is coherent, choose private implementation mechanics.
9. Implement the smallest coherent milestone.
10. Validate the contract and stop at that milestone.

Treat representative public code as an API regression check. If a proposed design makes normal use materially more complicated, justify that complexity from the contract.

Storage layout, bytecode, PIMPL, register allocation, caching, backend translation, container choice, and similar mechanics must not drive public design unless they impose a real externally visible constraint.

## Optimize an ambiguous prompt

Before implementation, reduce the task to this compact contract:

```text
Goal:
Observable semantics:
Invariants:
Allowed scope:
Non-goals:
Validation:
```

Ask only questions whose answers would materially change durable behavior. Typical high-leverage questions concern:

- the exact public abstraction and terminology;
- ownership, sharing, borrowing, and lifetime;
- mutability, thread confinement, and synchronization;
- error reporting, recovery, and exceptional states;
- ordering, units, coordinate systems, representations, or serialization;
- compatibility requirements and intentionally unsupported cases;
- what evidence proves the implementation correct.

Do not ask about choices that are already encoded in the target module, settled by repository instructions, mechanically inferable, or easily reversible without affecting public semantics.

When an interactive answer is unavailable, state the semantic assumption explicitly and avoid making an irreversible public API or architecture choice.

## Create a new module

For a new module, the first authored semantic file is `<module>/AGENTS.md`.

1. Draft it from `docs/module-agents-template.md`.
2. Fill only facts established by the task, existing architecture, or explicit user decisions.
3. Put unresolved semantic choices under `Open decisions`; do not invent answers to make the document look complete.
4. Confirm or otherwise establish the public model before implementing the API.
5. Implement the smallest end-to-end behavior that validates the model.

The module generator may create neutral boilerplate, but it must not fabricate a purpose, invariants, ownership model, or future architecture.

## Work in an existing module

Do not generate an `AGENTS.md` merely because one is absent. Add one only when the task exposes durable semantics that are important, non-obvious, and likely to matter in future work.

When a module contract already exists:

- preserve it unless the task explicitly changes it;
- treat `Open decisions` as questions, not permission to choose arbitrarily;
- update it only when the durable contract changes.

## Prevent low-value generation

- Prefer one direct implementation over a framework for possible future implementations.
- Do not create wrappers that merely rename an existing operation.
- Do not add generic parameters until the current task has a concrete second variation that requires them.
- Do not add comments that restate code.
- Do not add broad documentation when one invariant in the module contract is sufficient.
- Do not create large demonstration CLIs or test matrices unrelated to the acceptance criteria.
- Do not rewrite unaffected code into a preferred style.
- Preserve the user's chosen names even when another common term exists.

## Validate

Validation should be proportional to the semantic reach of the change:

- private implementation change: build and exercise the target module;
- public API or behavior change: also build or exercise affected direct dependents;
- graph, phase, process, or bootstrap change: run the foundation validation named by the relevant `AGENTS.md`;
- interactive, terminal, graphics, or hardware behavior: distinguish automated checks from manual checks.

Tests should prove observable contracts, invariants, and important negative cases rather than merely exercise code.

Report the exact commands run, exit results, and untested cases. A plausible explanation is not validation evidence.

## Record durable knowledge

Put enduring module-specific knowledge in the module's single `AGENTS.md`.
Put repository-wide graph, phase, artifact, or workspace knowledge in `docs/repository-model.md`.
Keep task plans, transcripts, temporary debugging notes, and migration narratives out of permanent instruction files.
