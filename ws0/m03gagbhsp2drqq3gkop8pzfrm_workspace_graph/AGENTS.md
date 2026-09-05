# `m03gagbhsp2drqq3gkop8pzfrm_workspace_graph`

## Purpose

Discover workspaces and modules for one workspace root, validate their identities, expose ordered workspace/module objects, derive source versions, and provide canonical source and artifact paths.

For source-include scanning, dependency eligibility, SCC construction, phase execution, and artifact publication, use the [foundation ownership table](../../docs/repository-model.md#foundation-ownership).

## Invariants

- Discovery indexes every direct module child before materializing modules on demand; module enumeration contains only materialized modules.
- The graph owns its workspace and module objects. Returned pointers and references are borrowed and remain valid only while their graph remains alive.
- Source and artifact paths are derived from the graph roots and module identity; callers must not reconstruct them independently.

## Validation

Extend the temporary workspace fixtures in `test/public_api.cpp` to provide evidence for discovery, identity, lifetime, version, or path behavior changed by the task.
