# `m03gagbhsujjf63n0w3r2w4q6h_build_phases`

## Goal

Materialize a module through source, interface, library, and binary phases; derive source dependencies; build library SCCs; dispatch exported producer entry points; validate outputs; and publish cache-keyed versioned artifacts. Keep dependency execution and publication within this phase lifecycle.

## Public model

A `builder.cpp` may implement module-specific behavior through exported `phase__<name>` entry points. The phase object passed to an entry point is borrowed for that call. Staging, installation, and output registration through it must complete synchronously before the entry point returns.

Each binary-phase invocation selects one target. `phase_base_t::make(module)` selects `cli`; the overload taking a target requires a non-empty name without path separators. `install_binary()` ignores other target names. A matching declaration must successfully build its output or validate a completed cache hit before the phase returns an installed result.

The binary installed result owns its selected target name and versioned install root. `root()` contains the executable and runtime artifacts; `target()` returns the selected executable path. Later publications do not change an earlier result's paths.

## Invariants

- A `built_t` retains its rooted artifact paths; phase APIs accept inputs from the current build or installed artifacts from an earlier phase.
- A binary invocation returns only the artifact resolved through its selected target's current declaration. Historical artifact or latest-link existence cannot resolve a target.
- Every member of a library SCC completes output construction and registered validation before any member is marked complete or published as latest.
- Producer symbol names and phase object layouts are plugin-ABI-sensitive.

## Validation

`test/public_api.cpp` covers the phase-object contract. For phase-semantic changes, bootstrap Builder and exercise only the affected end-to-end cases, including cache reuse, failure recovery, dependency cycles, or phase execution over later-workspace fixtures when those semantics change.
