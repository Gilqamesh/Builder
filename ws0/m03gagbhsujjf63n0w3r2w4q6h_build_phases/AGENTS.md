# `m03gagbhsujjf63n0w3r2w4q6h_build_phases`

## Purpose

Materialize a module through source, interface, library, and binary phases; derive source dependencies; build library SCCs; dispatch module-owned producer code; validate outputs; and publish cache-keyed versioned artifacts.

## Public model

A phase consumes installed artifacts from earlier phases and publishes its own installed artifacts. A `builder.cpp` may implement module-specific behavior through exported `phase__<name>` entry points; a missing entry point uses the default phase behavior.

## Invariants

- The phase chain is `source`, `interface`, `library`, `binary`.
- Build inputs come from earlier phase install roots or are explicitly staged as external inputs.
- `source` publishes the selected source view; downstream phases consume that installed view.
- Normal public interfaces are installed beneath the complete module-name include prefix; compatibility installation is explicit.
- Dependency interfaces and libraries are derived from module-qualified includes and their dependency closure.
- Library cycles are staged and validated as one SCC before any member is marked complete.
- `test/public_api.cpp` is registered as library validation when present; producers may register additional validations.
- Phase cache keys include the inputs relevant to that phase, including dependency source keys where applicable.
- A started marker without a complete marker is interrupted or re-entrant work and is not reused as complete.
- Failure removes the incomplete versioned artifact.
- `latest/<kind>` is published only after phase execution and output construction succeed.
- `cli.cpp` supplies the default binary target named `cli`; explicitly selected target names remain distinct artifact kinds.
- Producer symbol names and phase object layouts are plugin-ABI-sensitive.

## Boundary

This module owns dependency-driven build materialization and publication. Workspace/module identity belongs to `workspace_graph`; low-level include scanning and artifact-path operations belong to their dedicated foundation modules. Application semantics and agent execution remain outside the build-phase model.

## Validation

Build the module library to run `test/public_api.cpp`. For phase-semantic changes, bootstrap Builder; build library, header-only, CLI-only, named-target, and same-workspace-cycle cases as relevant; rebuild unchanged input to exercise cache reuse; and force a phase failure to verify cleanup and recovery. Rebuild representative later-workspace modules when public phase behavior changes.

## Direction required

Obtain direction before changing phase identity/order, default phase behavior, cache-key inputs, artifact layout, marker semantics, `latest` publication, source materialization, dependency closure/SCC behavior, producer loading, exported symbols, validation timing, or installed output contracts.
