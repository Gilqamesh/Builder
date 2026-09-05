# Repository model

This document is authoritative for intended cross-module and build-system semantics. Repository code remains authoritative for current behavior. Report mismatches and distinguish current facts from intended decisions.

## Repository layout

A Builder workspace root contains ordered workspace directories named `ws<N>`, where `N` is a non-negative decimal workspace position. Each direct child directory or symlink of a workspace identifies one module. A combined development workspace may therefore compose modules from several repositories without changing module identity.

The current convention is:

- `ws0`: Builder bootstrap and foundation modules;
- `ws1`: reusable libraries, representations, adapters, and development tools;
- `ws2`: applications, external integrations, and other higher-level modules.

Numeric workspace ordering determines dependency eligibility. The descriptive roles are architectural guidance.

## Agent documentation ownership

The root `AGENTS.md` and its routed documents apply to the combined development workspace, including modules supplied by another repository. Either repository may contain `<workspace>/<module>/AGENTS.md` for durable module-local semantics. There is no workspace-level `AGENTS.md` layer, and `Builder-Modules` does not duplicate the repository-wide documents.

## Module identity

A module directory name has this form:

```text
m<25-character-base36-encoded-UUIDv7>_<friendly_name>
```

The complete directory name is the module's stable identity. It is also the project namespace and the first component of every external include path. Friendly names are navigation aids and are not globally unique identities.

Use the complete directory name for module identity, namespaces, includes, and unambiguous execution. A module identity change requires explicit direction.

## Module boundaries

Apply these criteria when module creation or a boundary change is requested. Use an existing module when it already owns the required abstraction.

A separate module is warranted when its abstraction:

- has one coherent responsibility;
- owns meaningful semantics rather than merely grouping files;
- can be understood, implemented, and tested independently of its consumer;
- has enough complexity that separation clarifies dependencies and ownership.

Multiple consumers are not required; reuse is useful evidence. Keep tightly coupled concepts with their consumer when they lack independently understandable and testable semantics.

## Foundation ownership

Use these owners for the neighboring responsibilities below. Their module contracts or public interfaces describe the owned capability.

| Owner | Responsibility |
|---|---|
| [`m03gagbhsp2drqq3gkop8pzfrm_workspace_graph`](../ws0/m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/AGENTS.md) | Workspace/module identity and discovery, source versions, and canonical roots. |
| [`m03gn8rf3pe86v64vphnaam6rl_source_dependencies`](../ws0/m03gn8rf3pe86v64vphnaam6rl_source_dependencies/source_dependencies.h) | Source-include scanning, dependency eligibility, and dependency closure. |
| [`m03gagbhsujjf63n0w3r2w4q6h_build_phases`](../ws0/m03gagbhsujjf63n0w3r2w4q6h_build_phases/AGENTS.md) | Dependency-driven phase execution, library SCC construction and validation, and publication timing. |
| [`m03gn8rf3pe8dkpk1uwsemhhmd_artifact_store`](../ws0/m03gn8rf3pe8dkpk1uwsemhhmd_artifact_store/artifact_store.h) | Versioned artifact and marker paths, completed-artifact lookup, and atomic `latest` updates. |

## Dependency discovery

The source graph is the dependency declaration:

- module-qualified includes in each compiled source set and its recursively included local headers establish module dependencies for that output;
- module-qualified includes in `builder.cpp` establish builder dependencies;
- a module does not depend on itself through its own include prefix;
- this derived graph is the dependency source of truth.

Normal module dependencies may target the same workspace or an earlier workspace. Builder dependencies must target an earlier workspace, except for modules participating in the active `ws0` bootstrap group.

Library dependency cycles are built as strongly connected components. Builder stages and validates every library in the component before marking the group complete. Dependency closure and link inputs derive from the source graph rather than incidental source order.

Every module-qualified include names the repository module that owns that interface. When a repository module owns an abstraction over a system, platform, or third-party interface, consumers use that module boundary; otherwise external headers are not repository-module dependencies and follow the applicable language instructions.

## Versions and artifacts

A module's source version is the latest modification time in its source tree. Phase-specific cache keys combine the relevant source versions, local inputs, settings, and dependency source keys. Completed artifacts with the same cache key are reusable.

The default roots come from:

```text
BUILDER_WORKSPACE_ROOT
BUILDER_ARTIFACT_ROOT
```

Versioned artifacts use a UTC creation prefix and cache-key suffix:

```text
<artifact_root>/<module>/<kind>/<utc>-<hash>/build/
<artifact_root>/<module>/<kind>/<utc>-<hash>/install/
<artifact_root>/<module>/latest/<kind> -> <completed-versioned-artifact>
```

Phase kinds include `source`, `interface`, `library`, and `builder`; binary targets use `binary/<target>`. A `.started` marker denotes in-progress work and a `.complete` marker denotes reusable output. Each `latest` entry is atomically replaced only after successful publication. The versioned directory is the artifact identity.

Because source identity is currently timestamp-derived rather than content-hashed, parallel Git worktrees require separate `BUILDER_ARTIFACT_ROOT` values to prevent ambiguous cache reuse.

## Module producers and phases

Each module may contain:

- `builder.cpp`: the producer plugin for its build behavior;
- `cli.cpp`: the default executable entry point;
- public and private module source.

The current phase chain is fixed:

```text
source -> interface -> library -> binary
```

- `source` publishes the selected source tree.
- `interface` publishes `.h` and `.hpp` files beneath the complete module-name prefix by default.
- `library` builds and publishes library sources and runs registered validation; `test/public_api.cpp` is registered automatically when present.
- `binary` builds and publishes a selected executable target; `cli.cpp` supplies the default `cli` target when present.

A later phase consumes artifacts from earlier phase install roots rather than ambient source paths. External inputs must be deliberately staged into the current phase build root.

A phase uses started and complete markers. Directory existence alone does not establish completion. Failed work removes the incomplete versioned artifact, and `latest` is published only after output construction succeeds.

`builder.cpp` may export `phase__source`, `phase__interface`, `phase__library`, and `phase__binary` to replace the corresponding default behavior. Missing symbols use the default phase implementation. These symbol names and phase object interfaces form a plugin ABI.

## Bootstrap and module execution

`m03gagbhst621faiop1rztfkqp_builder_cli` is the bootstrap seed module in `ws0`. `m03gn8rf3peew0re4l1s2vvaw6_bootstrap_seed` owns the explicit set of modules compiled into that seed and the same-workspace builder-dependency exception for those modules.

The running CLI compares its modification time with the newest source version in the bootstrap set. When stale, it rebuilds and transfers execution through the active seed; otherwise it builds the requested module's selected binary target and executes it. The default target is `cli`.

`m03gf09la5rvbh6kk4vvt1qawv_module_shell` resolves friendly names for interactive navigation, but complete module names remain the unambiguous execution identity.

## Current direction

The agent workflow is documentation-dispatched: the root router selects global rules and, when present, one semantic contract for each affected module. Explicit module boundaries, source-derived dependencies, independently built artifacts, and small public interfaces provide the scaling mechanism for both implementation and multi-module dispatch.

Agent scheduling, IPC, network protocols, and generalized task-graph infrastructure are outside the current repository model.

## Decisions requiring explicit direction

Ask before changing any of these repository-wide semantics:

- module-name grammar or identity;
- workspace ordering rules;
- dependency discovery or SCC meaning;
- source-version and cache-key semantics;
- artifact layout or `latest` publication;
- phase order, default phase behavior, or producer ABI;
- bootstrap seed membership;
- the boundary between build-time composition and runtime module communication.
