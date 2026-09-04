# Repository instructions

`/AGENTS.md` is the canonical entry point for repository-wide design, change, context, and validation discipline. Read it before editing, then load only the workspace, module, shared document, and path-specific instruction files it dispatches for the task.

For C++ changes, apply the matching files under `.github/instructions/`. Preserve the complete module name in namespaces and external include paths, and do not bypass a module boundary to reach an implementation detail or underlying dependency.
