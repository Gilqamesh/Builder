# `m03gagbhsp2drqq3gkop8pzfrm_workspace_graph`

## Goal

Discover workspaces and modules for one workspace root, validate their identities, expose ordered workspace/module objects, derive source versions, and provide canonical source and artifact paths.

## Invariants

- Discovery indexes every direct module child before materializing modules on demand; module enumeration contains only materialized modules.
- The graph owns its workspace and module objects. Returned pointers and references are borrowed and remain valid only while their graph remains alive.
- Source and artifact paths are derived from the graph roots and module identity; use the graph-provided paths rather than reconstructing them independently.

## Validation

Extend the temporary workspace fixtures in `test/public_api.cpp` to provide evidence for discovery, identity, lifetime, version, or path behavior changed by the task.
