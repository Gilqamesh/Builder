# `m03gagbht5685jfnokvj7crv2c_create_module`

## Purpose

Create neutral module boilerplate in an existing workspace using a newly generated valid module identity.

This module owns neutral filesystem scaffolding. Use the [agent workflow](../../docs/agent-workflow.md#create-a-new-module) for semantic discovery and module-contract authoring. Package management, dependency solving, and architecture generation require separately requested semantic or architectural work.

## Invariants

- Module identity is generated through `module_name_t::from_friendly_name` rather than assembled manually.
- The target workspace must already exist and be a directory.
- Existing module directories or files are never overwritten.
- Generated namespaces and include guards use the complete module name.
- Generated `builder.cpp` is empty so the build system's default phase behavior applies.
- Generated `api.h` provides only the complete module namespace, and `cli.cpp` provides the neutral default CLI.
- Boilerplate remains semantically neutral: the generator must not invent the module's purpose, ownership model, invariants, dependencies, or future features.

## Validation

Extend `test/public_api.cpp` for changed generator behavior. For template changes, also build a generated module through the default phase chain and invoke its CLI.
