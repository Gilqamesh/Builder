# `m03gagbhsp2drqq3gkop8pzfrm_workspace_graph`

## Purpose

Discover workspaces and modules for one workspace root, validate their identities, expose ordered workspace/module objects, derive source versions, and provide canonical source and artifact paths.

This module owns workspace/module identity, discovery, source versions, and canonical roots. Source-include scanning, dependency eligibility, SCC construction, phase execution, and artifact publication belong to their dedicated foundation modules.

## Invariants

- Discovery indexes every direct module child before materializing modules on demand; module enumeration contains only materialized modules.
- The graph owns its workspace and module objects. Returned pointers and references are borrowed and remain valid only while their graph remains alive.
- Source and artifact paths are derived from the graph roots and module identity; callers must not reconstruct them independently.

## Validation

`test/public_api.cpp` is the executable contract. Extend its temporary workspace fixtures for discovery, identity, lifetime, version, or path behavior changed by the task.
