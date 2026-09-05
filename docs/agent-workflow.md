# Agent workflow

Use this workflow when a task creates a module, changes public semantics, crosses module boundaries, or leaves durable semantic choices unresolved.

## Classify the task

Classify the task before acquiring context:

- **Investigate or review:** inspect and report within the requested scope.
- **Local implementation:** preserve the existing module contract and implement the requested behavior.
- **Semantic or public API change:** settle the contract decisions being changed before implementation.
- **New module:** establish a module contract before defining its public interface when durable, non-obvious semantics warrant one.

## Acquire context

Follow the context dispatch and discipline in the root `AGENTS.md`. Let exact module boundaries focus repository searches, and stop once the requested behavior and governing constraints are understood. Keep future possibilities undecided until they impose a current semantic requirement, and preserve unaffected style and terminology.

## Review a public abstraction

For a new or materially changing public abstraction:

1. Identify its responsibility and semantic boundary.
2. Inspect the current implementation and existing dependency interfaces.
3. Write or inspect the smallest representative caller example.
4. Settle terminology, ownership, mutability, invariants, and observable behavior through [semantic decision handling](#settle-semantic-decisions).
5. Express the boundary through capabilities, inputs, outputs, and invariants; make the public model coherent before choosing private mechanics.
6. When implementation is requested and the necessary contract decisions are settled, implement the smallest coherent milestone.
7. Validate the contract and stop at that milestone.

Apply the relevant language instructions for concrete API and coding rules. Keep private implementation mechanics out of the public model unless they impose observable behavior.

## Settle semantic decisions

When a prompt leaves durable behavior ambiguous, use only the applicable fields from this compact contract and omit the rest:

```text
Goal:
Observable semantics:
Invariants:
Boundary:
Validation:
```

When useful, supplement this task-local contract with non-goals, ordered priorities, or whether a material choice is fixed, open, or explicitly delegated. These are not mandatory fields or a persistent decision ledger.

Consider only dimensions raised by the task, an authoritative existing contract, or a current dependency. Ask only questions whose answers materially change durable behavior:

- the exact public abstraction and terminology;
- ownership, sharing, borrowing, lifetime, or mutability when they affect public use;
- error reporting, recovery, and exceptional states;
- ordering, units, coordinate systems, representations, or serialization;
- established compatibility requirements;
- what evidence proves the implementation correct.

Before asking the user to settle a material design choice, explain the concrete problem and show its context through concise code excerpts. Include the caller, public interface, and adjacent implementation or pipeline stages only as needed to explain the consequences. For viable alternatives, show the smallest meaningful code differences, explain the tradeoffs, and recommend an option before asking the specific unresolved question. Distinguish current code from proposed sketches. Expose ownership, lifetime, ordering, and data flow where relevant; omit unrelated details and complete implementations.

Treat choices established by authoritative public contracts, module contracts, repository instructions, or explicit task direction as fixed. Implement settled semantics; a broad request to audit, harden, correct, reconcile, or improve safety does not delegate a materially different observable contract.

Before production changes, surface any assumption or implementation constraint that would select unresolved public semantics, including accepted types or values, identity or equivalence, ownership or lifetime, mutability or access, copyability, or failure behavior. Obtain direction for that choice unless the user explicitly delegates it. Needing a choice to proceed is not delegation.

Make mechanically reversible implementation choices within the settled contract. Continue independent work while a material choice remains open; leave production changes that resolve or depend on it pending. Report unresolved choices when direction is unavailable. Tests provide evidence for settled contracts; they do not establish them.

## Create a new module

Apply the [module-boundary criteria](repository-model.md#module-boundaries) to requested module creation. When a new module has durable, non-obvious semantics that warrant a module contract, draft `<module>/AGENTS.md` before designing its public API. Apply `docs/module-agents-template.md` as a filter, record only direction established by the task or authoritative architecture, and put unresolved semantic choices under `Open decisions`.

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

The coordinating agent handles cross-module choices through [semantic decision handling](#settle-semantic-decisions) and reconciles module changes against the shared contract before final validation. Partitioning follows existing module ownership; it does not introduce an agent runtime, scheduler, protocol, or task-graph implementation.

## Validate

Match validation to the semantic reach of the change:

- private implementation change: build and exercise the target module;
- public API or behavior change: also build or exercise affected direct dependents;
- graph, phase, process, or bootstrap change: run the foundation validation named by the relevant `AGENTS.md`;
- interactive, terminal, graphics, or hardware behavior: distinguish automated checks from manual checks.

Use tests as executable evidence for observable contracts, invariants, and relevant negative cases.

Report the exact commands run, their results, and untested cases. Only obtained evidence counts as validation.

## Record durable knowledge

Follow the root [documentation ownership](../AGENTS.md#documentation-ownership) and [placement rules](../AGENTS.md#semantic-control), and apply the [module-contract filter](module-agents-template.md) before recording module-specific direction.
