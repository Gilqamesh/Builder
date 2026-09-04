# `m03gagbht5685jfnokvj7crv2c_create_module`

## Purpose

Create neutral module boilerplate in an existing workspace using a newly generated valid module identity.

This module owns neutral filesystem scaffolding only. The agent workflow owns semantic discovery and module-contract authoring; package management, dependency solving, and architecture generation belong elsewhere.

## Invariants

- Module identity is generated through `module_name_t::from_friendly_name` rather than assembled manually.
- The target workspace must already exist and be a directory.
- Existing module directories or files are never overwritten.
- Generated namespaces and include guards use the complete module name.
- Generated `builder.cpp` is empty so the build system's default phase behavior applies.
- Generated `api.h` provides only the complete module namespace, and `cli.cpp` provides the neutral default CLI.
- Boilerplate remains semantically neutral: the generator must not invent the module's purpose, ownership model, invariants, dependencies, or future features.

## Validation

`test/public_api.cpp` verifies generated identities and exact neutral contents, invalid friendly names, invalid or missing workspace paths, and refusal to overwrite. For template changes, also build a generated module through the default phase chain and invoke its CLI.
