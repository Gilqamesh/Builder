# `m03gagbhsp2drqq3gkop8pzfrm_workspace_graph`

## Purpose

Discover workspaces and modules for one workspace root, validate their identities, expose ordered workspace/module objects, derive source versions, and provide canonical source and artifact paths.

## Invariants

- Workspace directories are named `ws<N>` and ordered by numeric `N`.
- Module names are globally unique and retain their complete UUIDv7-based identity.
- Discovery indexes every direct module child before materializing modules on demand.
- A module's source version is the latest modification time in its source tree.
- Source and artifact paths are derived from the graph roots and module identity; callers must not reconstruct them independently.

## Boundary

This module owns workspace/module identity, discovery, source versions, and canonical roots. Source-include scanning, dependency eligibility, SCC construction, phase execution, and artifact publication belong to their dedicated foundation modules.

## Validation

Build the module library to run `test/public_api.cpp`. Cover module-name parsing/generation, workspace-name ordering, duplicate module identity, lazy discovery, source-version calculation, invocation-root selection, and derived source/artifact paths as applicable to the change.

## Direction required

Obtain direction before changing module-name grammar, directory discovery, workspace-name ordering, source-version meaning, invocation-root defaults, or canonical source/artifact path semantics.
