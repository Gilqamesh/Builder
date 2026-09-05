# `m03gagbhsujjf63n0w3r2w4q6h_build_phases`

## Purpose

Materialize a module through source, interface, library, and binary phases; derive source dependencies; build library SCCs; dispatch module-owned producer code; validate outputs; and publish cache-keyed versioned artifacts.

For workspace/module identity, source-include scanning and eligibility, and artifact-path operations, use the [foundation ownership table](../../docs/repository-model.md#foundation-ownership). Application semantics and agent execution remain outside the build-phase model.

## Public model

A `builder.cpp` may implement module-specific behavior through exported `phase__<name>` entry points. The phase object passed to an entry point is borrowed for that call; a producer must synchronously stage, install, or register its outputs through that object.

## Invariants

- A `built_t` retains its rooted artifact paths; phase APIs accept inputs from the current build or installed artifacts from an earlier phase.
- Every member of a library SCC completes output construction and registered validation before any member is marked complete or published as latest.
- Producer symbol names and phase object layouts are plugin-ABI-sensitive.

## Validation

`test/public_api.cpp` covers the phase-object contract. For phase-semantic changes, bootstrap Builder and exercise only the affected end-to-end cases, including cache reuse, failure recovery, dependency cycles, or representative later-workspace modules when those semantics change.
