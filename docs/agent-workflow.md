# Agent workflow

Use this workflow when a task creates a module, changes public semantics, crosses module boundaries, or leaves durable semantic choices unresolved.

## Classify the task

Classify the task before acquiring context:

- **Investigate or review:** inspect and report within the requested scope.
- **Local implementation:** preserve the existing module contract and implement the requested behavior.
- **Semantic or public API change:** settle the contract decisions being changed before implementation.
- **New module:** define the module contract before defining its public interface.

Keep a local implementation within its established semantic boundary.

## Acquire context

1. Resolve the exact target module from its path or complete module name.
2. Follow the dispatch rules in the root `AGENTS.md`.
3. Read the applicable language instructions.
4. Read the target module's public headers, then its `AGENTS.md` when present.
5. Read only the implementation and validation paths relevant to the task.
6. Read direct dependency interfaces only when the target code uses them.
7. Stop acquiring context once the requested behavior and constraints are understood.

Let module boundaries focus repository searches and keep context proportional to the change.

## Review a public abstraction

For a new or materially changing public abstraction:

1. Identify its responsibility and semantic boundary.
2. Inspect the current implementation and existing dependency interfaces.
3. Write or inspect the smallest representative user-facing use.
4. Settle terminology, ownership, mutability, invariants, and observable behavior.
5. State the boundary and adjacent responsibilities owned elsewhere.
6. Decide whether an existing module owns part of the problem.
7. Decide whether a new module or abstraction is warranted under `docs/repository-model.md`.
8. Make the public model coherent before choosing private implementation mechanics.
9. Implement the smallest coherent milestone.
10. Validate the contract and stop at that milestone.

The public API must be concise, technically precise, and understandable from representative user-authored code. Additional complexity in normal use requires a concrete semantic need.

Implementation mechanics such as storage layout, bytecode, PIMPL, register allocation, caching, backend translation, and container choice remain private unless they impose an observable constraint.

Ownership, borrowing, lifetime, mutability, error behavior, and successful-construction invariants must be explicit and intentional. Immutable descriptions or programs remain separate from mutable per-use state when they are different concepts. Backend-neutral interfaces express backend-neutral semantics, with backend mechanics handled in backend-private translation. Generic or shared abstractions require a concrete shared semantic need.

Collaborating types communicate through ordinary interfaces aligned with ownership boundaries. Structure the API so friendship, passkeys, and privileged access shims are unnecessary.

## Optimize an ambiguous prompt

When a prompt leaves durable behavior ambiguous, reduce it to this compact contract:

```text
Goal:
Observable semantics:
Invariants:
Allowed scope:
Non-goals:
Validation:
```

Ask only questions whose answers materially change durable behavior. High-leverage questions concern:

- the exact public abstraction and terminology;
- ownership, sharing, borrowing, and lifetime;
- mutability, thread confinement, and synchronization;
- error reporting, recovery, and exceptional states;
- ordering, units, coordinate systems, representations, or serialization;
- compatibility requirements and intentionally unsupported cases;
- what evidence proves the implementation correct.

Infer choices already encoded in the target module, settled by repository instructions, or mechanically reversible without changing public semantics.

When an interactive answer is unavailable, state the semantic assumption and keep irreversible public API or architecture choices open.

## Create a new module

For a new module, the first authored semantic file is `<module>/AGENTS.md`.

1. Draft it from `docs/module-agents-template.md`.
2. Fill only facts established by the task, existing architecture, or explicit user decisions.
3. Put unresolved semantic choices under `Open decisions`.
4. Establish the public model before implementing the API.
5. Implement the smallest end-to-end behavior that validates the model.

The module generator creates neutral boilerplate. Purpose, invariants, ownership, and architecture come from the task and explicit design decisions.

## Work in an existing module

A missing `AGENTS.md` is normal. Add one only when the task establishes important, durable, non-obvious semantics for future work.

When a module contract already exists:

- Preserve it unless the task explicitly changes it.
- Treat `Open decisions` as questions requiring direction.
- Update it only when the durable contract changes.

## Dispatch work across modules

For a task spanning multiple independent modules, settle shared contracts and cross-module semantic decisions first. Then partition independent implementation work by complete module boundary.

Give each module worker only:

- the shared task and settled contract;
- applicable global instructions;
- its module contract;
- its public and relevant implementation files;
- public interfaces of direct dependencies.

Keep cross-module architectural decisions with the coordinating agent. Reconcile the resulting module changes against the shared contract before final validation. Partitioning follows existing module ownership; it does not introduce an agent runtime, scheduler, protocol, or task-graph implementation.

## Choose the smallest coherent design

- Every public concept, extension point, and generic parameter must serve a concrete current use.
- Future implementation possibilities remain private or undecided until they impose a real semantic requirement.
- One direct implementation is preferable when the current contract has one concrete behavior.
- Comments explain non-obvious intent; module contracts record only durable semantics.
- Demonstrations and test matrices remain proportional to the acceptance criteria.
- Unaffected code retains its established style and terminology.

## Validate

Match validation to the semantic reach of the change:

- private implementation change: build and exercise the target module;
- public API or behavior change: also build or exercise affected direct dependents;
- graph, phase, process, or bootstrap change: run the foundation validation named by the relevant `AGENTS.md`;
- interactive, terminal, graphics, or hardware behavior: distinguish automated checks from manual checks.

Tests must prove observable contracts, invariants, and relevant negative cases.

Report the exact commands run, their results, and untested cases. Only obtained evidence counts as validation.

## Record durable knowledge

- Put enduring module-specific knowledge in the module's single `AGENTS.md`.
- Put repository-wide module, workspace, dependency, phase, artifact, and bootstrap semantics in `docs/repository-model.md`.
- Keep temporary task material outside permanent instruction files.
