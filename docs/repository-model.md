# Repository model

This document is authoritative for the intended cross-module architecture. Repository code remains authoritative for current behavior. Report mismatches instead of silently treating either one as the other.

## Repository layout

The repository root contains ordered workspace directories named `ws<N>`, where `N` is a non-negative decimal workspace position. Each direct child directory of a workspace is a module.

The current convention is:

- `ws0`: Builder bootstrap and foundation modules;
- `ws1`: reusable libraries, representations, adapters, and development tools;
- `ws2`: applications, external integrations, and higher-level experiments.

Only the numeric workspace ordering is enforced by the graph. The descriptive roles above are architectural guidance.

## Module identity

A module directory name has this form:

```text
m<25-character-base36-encoded-UUIDv7>_<friendly_name>
```

The complete directory name is the module's stable identity. It is also the project namespace and the first component of every external include path. Friendly names are navigation aids and are not globally unique identities.

Do not rename a module, regenerate its identifier, shorten its namespace, or reconstruct its name from the friendly suffix unless the task explicitly changes module identity.

## Module boundaries

A module does not need multiple consumers to be justified. A separate module is warranted when an abstraction:

- has one coherent responsibility;
- owns meaningful semantics rather than merely grouping files;
- can be understood and tested independently of its consumer;
- has enough complexity that separation clarifies dependencies and ownership.

Reuse is useful evidence, not a requirement. At the same time, avoid micro-modularization: do not split tightly coupled concepts merely because they occupy multiple classes or files.

Use an existing module as a dependency when it genuinely owns the required abstraction. Do not duplicate that functionality locally.

> Can this abstraction be understood, implemented, and tested independently of its consumer?

If yes and it owns meaningful complexity, consider a module. If not, keep it within the consumer.

## Dependency discovery

The source graph is the dependency declaration:

- module-qualified includes in ordinary `.h`, `.hpp`, `.c`, and `.cpp` source establish module dependencies;
- module-qualified includes in `builder.cpp` establish builder dependencies;
- a module does not depend on itself through its own include prefix;
- there is no second handwritten dependency manifest to keep synchronized.

Normal module dependencies may target the same workspace or an earlier workspace. Builder dependencies must target an earlier workspace, except for modules participating in the active `ws0` bootstrap group.

Normal dependency cycles are represented as strongly connected components. Dependency closures are exposed as SCC groups in dependency-to-dependent topological order; link ordering and static-library grouping derive from this graph rather than incidental source order.

Before adding an include, identify the module that owns the interface. Do not bypass a boundary by including an underlying C, C++, system, platform, or third-party header directly.

## Versions and artifacts

A module begins with a version derived from the latest modification time in its source tree. Dependency and builder-dependency versions propagate into the effective module version.

The default roots come from:

```text
BUILDER_WORKSPACE_ROOT
BUILDER_ARTIFACT_ROOT
```

The current artifact layout is:

```text
<artifact_root>/<module>/<module>@<version>/
<artifact_root>/<module>/latest/
```

`latest` is a convenience view whose phase entries are atomically replaced after successful phase publication. The versioned directory is the real artifact identity.

Because versions are currently timestamp-derived, parallel Git worktrees should use separate `BUILDER_ARTIFACT_ROOT` values. Do not assume one shared artifact cache distinguishes simultaneous source states by content.

## Module producers and phases

Each module may contain:

- `builder.cpp`: the producer plugin for its build behavior;
- `cli.cpp`: the default executable entry point;
- public and private module source.

The default phase chain is:

```text
SOURCE -> INTERFACE -> LIBRARY -> BINARY
```

The configured phase order is explicit and must be non-empty and free of duplicate phase identifiers.

- `SOURCE` publishes the selected source tree as a phase artifact.
- `INTERFACE` publishes public include artifacts, normally under the complete module-name prefix.
- `LIBRARY` builds and publishes the module library when the module has library sources.
- `BINARY` builds and publishes the default CLI as `cli` when applicable.

A later phase consumes artifacts from earlier phase install roots rather than ambient source paths. External inputs must be deliberately staged into the current phase build root.

A phase uses started and complete markers. Completion is not inferred from directory existence. Failed work removes incomplete configured build and install roots; the `latest` view is published only after typed output construction succeeds.

`builder.cpp` exports phase entry points such as `phase__source`, `phase__interface`, `phase__library`, and `phase__binary`. Treat those symbol names and phase object interfaces as a plugin ABI.

## Bootstrap and module execution

`m03gagbhst621faiop1rztfkqp_builder_cli` is the bootstrap seed module in `ws0`. The bootstrap CLI rebuilds through the active seed when the running CLI is older than the seed, then builds the requested module's binary phase and executes its default CLI.

`m03gf09la5rvbh6kk4vvt1qawv_module_shell` resolves friendly names for interactive navigation, but complete module names remain the unambiguous execution identity.

## Sources of truth

- Repository code, including public headers, implementation, and executable validation, is authoritative for current behavior.
- The target module's `AGENTS.md` is authoritative for its documented intended semantics and invariants.
- This document is authoritative for intended cross-module architecture.
- The current task may explicitly change current or intended behavior.

When these sources disagree, report the mismatch. Distinguish current implementation facts, settled architectural decisions, open decisions, and possible future directions. Transitional or stale code may describe what currently happens, but it must not silently become an architectural constraint. Historical or future planning is not a current requirement unless the task makes it one.

## Current direction

The immediate agentic workflow is documentation-dispatched: the root router selects global rules, a workspace guide, and at most one semantic contract per module. Do not introduce an agent runtime, task scheduler, IPC layer, network protocol, or generalized execution graph unless the task explicitly asks for that infrastructure.

Structured module communication remains a plausible future direction because it could support agents, evaluators, and result composition. It is not a current invariant and must not be used to justify speculative APIs today.

Preserve the existing natural scaling mechanism: explicit module boundaries, graph-derived dependency ordering, independently built artifacts, and small public interfaces.

## Decisions requiring explicit direction

Ask before changing any of these repository-wide semantics:

- module-name grammar or identity;
- workspace ordering rules;
- dependency discovery or SCC meaning;
- module version propagation;
- artifact layout or `latest` publication;
- default phase order or producer ABI;
- bootstrap seed membership;
- the boundary between build-time composition and runtime module communication.
