# Agent workflow

Use this workflow when a task creates a module, changes public semantics, crosses module boundaries, or leaves durable semantic choices unresolved.

## Classify the task

Classify the task before acquiring context:

- **Investigate or review:** inspect and report within the requested scope.
  When investigating a possible change, explain the relevant observations and
  sources, important unknowns, and plausible approaches with their tradeoffs.
  Recommend the smallest next step and justify how it would advance the goal
  or resolve uncertainty.
- **Local implementation:** preserve the existing module contract and implement the requested behavior.
- **Semantic or public API change:** settle the contract decisions being changed before implementation.
- **New module:** establish the module README Goal before designing its public API, then record the model and requirements needed to define the contract.

## Logical change record

Organize planning and completion around this record:

```makefile
Goal:
The problem and intended observable outcome.

Changes:
The resulting changes, relevant before/after behavior, and significant rationale.

Validation:
Checks performed, actual results, and remaining uncertainty.
```

During planning, state the intended changes and checks in the same structure.
Derive the task Goal from the requested outcome and affected module README Goals;
update a module's Goal only when its purpose changes. Under Changes, include caller
or data migration steps when required. Identify missing semantic decisions and
continue independent work while those decisions remain open.

At completion, replace planned changes and checks with the resulting changes and
actual evidence. Include changed files, exact validation commands and results,
unresolved semantic decisions, and unverified behavior. A proposed check is not a
completed check.

Present the completed record according to the root [completion rules](../AGENTS.md#completion).
When committing, also include it in the logical change's commit message, including
the squash commit when applicable. A PR description can carry the working record;
update it as the change develops. For a change spanning Builder and Builder-Modules,
identify the corresponding change in the other repository in each record. Do the
same for any additional participating repository. Use stable branch, commit, or PR
references so the coordinated change can be found.

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
7. Validate the contract and stop at that milestone unless the task authorizes further milestones.

Apply the relevant language instructions for concrete API and coding rules. Keep private implementation mechanics out of the public model unless they impose observable behavior.

## Settle semantic decisions

When reviewing a policy, identify the requirement it serves. Describe its current implementation separately, and state when the rationale is undocumented.

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

Before asking for a semantic decision, show the smallest caller or code excerpt needed to make the choice concrete. Explain the behavioral tradeoffs, recommend an option, and ask for the missing decision. Distinguish current behavior from the proposal.

Treat choices established by authoritative public contracts, module contracts, repository instructions, or explicit task direction as fixed. Implement settled semantics; a broad request to audit, harden, correct, reconcile, or improve safety does not delegate a materially different observable contract.

Before production changes, surface any assumption or implementation constraint that would select unresolved public semantics, including accepted types or values, identity or equivalence, ownership or lifetime, mutability or access, copyability, or failure behavior. Obtain direction for that choice unless the user explicitly delegates it. Needing a choice to proceed is not delegation.

Make mechanically reversible implementation choices within the settled contract. Continue independent work while a material choice remains open; leave production changes that resolve or depend on it pending. Report unresolved choices when direction is unavailable. Tests provide evidence for settled contracts; they do not establish them.

## Create a new module

Apply the [module-boundary criteria](repository-model.md#module-boundaries) to requested module creation. Establish the module's Goal from the task's settled intent before designing its public API. Draft the Goal for `<module>/README.md` using the [module README convention](module-readme.md), describing the capability, why someone needs it, and its responsibility boundary. Ask for direction on missing durable semantics; do not infer purpose from the generated directory name.

Add model and requirements only when useful, record only direction established by the task or authoritative contracts, and keep unresolved semantic choices explicitly open. Establish the public model before implementing the API, then implement the smallest end-to-end behavior that validates the model. Write and review the README as part of completing module creation, including relevant usage, limitations, or validation.

`create_module` creates neutral filesystem scaffolding; it does not establish module semantics or invent a Goal. The authoring workflow supplies the Goal and README from the task's established intent.

## Work in an existing module

Read the README Goal and relevant contracts before planning changes. Preserve the current purpose unless the task explicitly changes it; update the Goal when the module's purpose changes. Preserve settled model and requirements, treat explicitly open decisions as questions requiring direction, and update the relevant contract when its semantics change. Local refactors that preserve behavior need no Goal change.

## Dispatch work across modules

For a task spanning multiple independent modules, settle shared contracts and cross-module semantic decisions first. Then partition independent implementation work by complete module boundary.

Give each module worker only:

- the shared task and settled contract;
- applicable global instructions;
- its module README Goal and relevant contracts;
- its public and relevant implementation files;
- public interfaces of direct dependencies.

The coordinating agent handles cross-module choices through [semantic decision handling](#settle-semantic-decisions) and reconciles module changes against the shared contract before final validation. Partitioning follows existing module ownership; it does not introduce an agent runtime, scheduler, protocol, or task-graph implementation.

## Validate

Use Builder's native C++ validation targets for framework tests. Avoid introducing Python scripts or a Python dependency for framework builds and validation.

Use the repository model for [phase execution and artifact publication](repository-model.md#module-producers-and-phases) and [artifact cache reuse](repository-model.md#versions-and-artifacts).

Match validation to the semantic reach of the change:

- private implementation change: build and exercise the target module;
- public API or behavior change: also build or exercise affected direct dependents;
- graph, phase, process, or bootstrap change: run the foundation validation named by the relevant module README;
- documentation change: apply the [module README convention](module-readme.md) and [module documentation invariant](repository-model.md#module-documentation), check semantic accuracy against the authoritative contract and current implementation, verify links, and check consistency between examples and the documented behavior; run existing example validation when an executable example changes;
- interactive, terminal, graphics, or hardware behavior: distinguish automated checks from manual checks.

Match evidence to the claimed outcome: use tests for observable contracts,
invariants, and relevant negative cases; before-and-after measurements for
performance claims; and representative callers or observations of use for API
usability and clarity. Relate the results to the intended improvement and state
the limits of what they establish.

Report the exact commands run, their results, and untested cases. Only obtained evidence counts as validation. Keep acceptance criteria intact and distinguish passing implementation checks from outcomes that still require external evidence. Continue authorized work that does not depend on missing evidence. For work authorized across multiple milestones, proceed when the current milestone's acceptance criteria pass and report any obstacle preventing further work.

## Record durable knowledge

Follow the root [documentation ownership](../AGENTS.md#documentation-ownership) and [placement rules](../AGENTS.md#semantic-control), and apply the [module README convention](module-readme.md) before recording module-specific direction. Keep task plans and completed change records outside permanent instruction files; report completed records according to the [logical change record](#logical-change-record).
