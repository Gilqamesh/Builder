# `m03gagbhsujjf63n0w3r2w4q6h_build_phases`

## Goal

Materialize a module through source, interface, library, and binary phases; derive source dependencies; build library SCCs; dispatch exported producer entry points; validate outputs; and publish cache-keyed versioned artifacts. Keep dependency execution and publication within this phase lifecycle.

## Public model

A `builder.cpp` may implement module-specific behavior through exported `phase__<name>` entry points. The phase object passed to an entry point is borrowed for that call. Staging, installation, and output registration through it must complete synchronously before the entry point returns.

## Invariants

- A `built_t` retains its rooted artifact paths; phase APIs accept inputs from the current build or installed artifacts from an earlier phase.
- Every member of a library SCC completes output construction and registered validation before any member is marked complete or published as latest.
- Producer symbol names and phase object layouts are plugin-ABI-sensitive.

## Validation

`test/public_api.cpp` covers the phase-object contract. For phase-semantic changes, bootstrap Builder and exercise only the affected end-to-end cases, including cache reuse, failure recovery, dependency cycles, or phase execution over later-workspace fixtures when those semantics change.
