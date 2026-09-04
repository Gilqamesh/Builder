# `m03gagbhsujjf63n0w3r2w4q6h_build_phases`

## Purpose

Materialize a module through configured source, interface, library, and binary phases, dispatch module-owned producer code, cache completed outputs, and publish versioned and `latest` artifacts.

## Public model

A phase consumes installed artifacts from earlier configured phases and publishes its own installed artifacts. `builder.cpp` implements module-specific phase behavior through exported `phase__<name>` entry points.

## Invariants

- The default order is `SOURCE`, `INTERFACE`, `LIBRARY`, `BINARY`.
- A configured phase order is non-empty and contains no duplicate phase identifiers.
- Build inputs come from earlier phase install roots or are explicitly staged as external inputs.
- `SOURCE` publishes the selected source view; downstream phases do not treat the raw workspace tree as an interchangeable artifact.
- Normal public interfaces are installed beneath the complete module-name include prefix; compatibility installation is explicit.
- Dependency interfaces and libraries are derived from the graph closure, including SCC-aware link ordering.
- A started marker without a complete marker is interrupted or re-entrant work and is not reused as complete.
- Failure removes incomplete configured build/install roots.
- `latest/<phase>` is published only after phase execution and typed output construction succeed.
- The default binary artifact is named `cli`.
- Producer symbol names and phase object layouts are plugin-ABI-sensitive.

## Non-goals

This module does not discover source dependencies, decide module semantics, or provide a general task/agent execution engine.

## Validation

Bootstrap Builder, build a module with headers and a library, build a CLI-only or header-only module, rebuild without source changes to exercise reuse, and force one phase failure to verify cleanup and recovery. Public phase changes also require rebuilding representative modules from later workspaces.

## Explicit decisions

Obtain direction before changing phase identity/order, phase directory layout, marker semantics, latest publication, source materialization, dependency closure aggregation, producer loading, exported symbols, or installed output contracts.
