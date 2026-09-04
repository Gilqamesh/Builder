# Agent workflow

Use this workflow when a task creates a module, changes public semantics, crosses module boundaries, or leaves durable semantic choices unresolved.

## Classify the task

Classify the task before acquiring context:

- **Investigate or review:** inspect and report within the requested scope.
- **Local implementation:** preserve the existing module contract and implement the requested behavior.
- **Semantic or public API change:** settle the contract decisions being changed before implementation.
- **New module:** establish a module contract before defining its public interface when durable, non-obvious semantics warrant one.

Keep a local implementation within its established semantic boundary.

## Acquire context

1. Resolve the exact target module from its path or complete module name.
2. Follow the dispatch rules in the root `AGENTS.md`.
3. Read the applicable language instructions.
4. Read the target module's `AGENTS.md` when present, then its public headers.
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

Apply the relevant language instructions for concrete API and coding rules. Keep private implementation mechanics out of the public model unless they impose observable behavior.

## Optimize an ambiguous prompt

When a prompt leaves durable behavior ambiguous, use only the applicable fields from this compact contract and omit the rest:

```text
Goal:
Observable semantics:
Invariants:
Boundary:
Validation:
```

Consider only dimensions raised by the task, an authoritative existing contract, or a current dependency. Ask only questions whose answers materially change durable behavior:

- the exact public abstraction and terminology;
- ownership, sharing, borrowing, lifetime, or mutability when they affect public use;
- error reporting, recovery, and exceptional states;
- ordering, units, coordinate systems, representations, or serialization;
- established compatibility requirements;
- what evidence proves the implementation correct.

Use choices already authoritative in public contracts, module contracts, repository instructions, or explicit task decisions. Make mechanically reversible implementation choices as needed without presenting them as durable semantics.

When an interactive answer is unavailable, state the semantic assumption and keep irreversible public API or architecture choices open.

## Create a new module

When a new module has durable, non-obvious semantics that warrant a module contract, draft `<module>/AGENTS.md` before designing its public API. Apply `docs/module-agents-template.md` as a filter, record only direction established by the task or authoritative architecture, and put unresolved semantic choices under `Open decisions`.

For a simple wrapper, adapter, generated module, or otherwise self-explanatory module, proceed directly to public-model design. In either case, establish the public model before implementing the API, then implement the smallest end-to-end behavior that validates the model.

The module generator creates neutral boilerplate; it does not establish module semantics.

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

Implement the smallest coherent milestone supported by the current contract. Keep future possibilities undecided until they impose a current semantic requirement, and preserve unaffected style and terminology.

## Validate

Match validation to the semantic reach of the change:

- private implementation change: build and exercise the target module;
- public API or behavior change: also build or exercise affected direct dependents;
- graph, phase, process, or bootstrap change: run the foundation validation named by the relevant `AGENTS.md`;
- interactive, terminal, graphics, or hardware behavior: distinguish automated checks from manual checks.

Tests must prove observable contracts, invariants, and relevant negative cases.

Report the exact commands run, their results, and untested cases. Only obtained evidence counts as validation.

## Record durable knowledge

- Apply the filter in `docs/module-agents-template.md` before recording module-specific direction.
- Put repository-wide module, workspace, dependency, phase, artifact, and bootstrap semantics in `docs/repository-model.md`.
- Keep temporary task material outside permanent instruction files.
