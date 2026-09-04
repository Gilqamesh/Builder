# `m03gagbhsp2drqq3gkop8pzfrm_workspace_graph`

## Purpose

Discover workspaces and modules, derive normal and builder dependency edges from source, validate workspace ordering, compute dependency SCCs and closures, propagate effective versions, and provide canonical source/artifact paths.

## Invariants

- Workspace directories are named `ws<N>` and ordered by numeric `N`.
- Module names are globally unique and retain their complete UUIDv7-based identity.
- Ordinary module dependencies come from module-qualified includes outside `builder.cpp`.
- Builder dependencies come from module-qualified includes in `builder.cpp`.
- Normal dependencies may target the same or an earlier workspace.
- Builder dependencies target an earlier workspace except within the explicitly recognized active bootstrap group.
- Normal dependency cycles are represented as SCCs; closure groups are dependency-to-dependent topological order.
- Effective versions include source-tree modification time and propagated dependency and builder-dependency versions.
- Source and artifact paths are derived from the graph roots and module identity; callers must not reconstruct them independently.
- The graph is the dependency source of truth. Do not add a second handwritten dependency manifest.

## Non-goals

This module does not compile code, execute phases, load producer plugins, or define runtime communication between arbitrary modules.

## Validation

After a change:

1. Bootstrap Builder.
2. Discover and build one `ws0`, one `ws1`, and one `ws2` target.
3. Exercise a target with same-workspace dependencies.
4. Exercise graph rendering or dependency-IR conversion when the public graph model changes.
5. Add focused invalid-layout cases for changed name, workspace, duplicate, or ordering rules.

## Explicit decisions

Obtain direction before changing module-name grammar, directory discovery, include parsing, workspace ordering, SCC grouping, version propagation, bootstrap recognition, or artifact path semantics.
