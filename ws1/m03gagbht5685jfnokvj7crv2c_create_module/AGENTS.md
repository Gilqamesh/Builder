# `m03gagbht5685jfnokvj7crv2c_create_module`

## Purpose

Create neutral module boilerplate in an existing workspace using a newly generated valid module identity.

## Invariants

- Module identity is generated through `module_name_t::from_friendly_name` rather than assembled manually.
- The target workspace must already exist and be a directory.
- Existing module directories or files are never overwritten.
- Generated namespaces and include guards use the complete module name.
- Generated `builder.cpp` follows the current default phase protocol.
- Boilerplate remains semantically neutral: the generator must not invent the module's purpose, ownership model, invariants, dependencies, or future features.

## Agent-aware module creation

The agent workflow, not boilerplate generation, owns semantic discovery. When an agent creates a real module, it should draft `<new-module>/AGENTS.md` from `docs/module-agents-template.md` before implementing the public API.

If this generator later creates an `AGENTS.md`, it may create only a concise template with explicit open decisions. It must not generate plausible semantic content from the friendly name.

## Non-goals

Do not make this module a package manager, dependency solver, architecture generator, or project wizard without explicit direction.

## Validation

Create a module in a temporary workspace, verify every generated path and identifier, build its default phase chain, invoke its CLI, and verify that a second creation attempt fails without altering existing files.
