Carry out a project-wide API documentation conciseness pass in:

/home/gilqamesh/Projects/Builder-Layout

Use the existing top-level repository documentation as authority: AGENTS.md, its dispatched documents, applicable .github/instructions files, and module-local AGENTS.md files. Focus on existing public API documentation, especially the overview comments attached to public abstractions.

Use the previous workflow: fresh contexts handling at most five distinct modules each, with separate per-module results. Put new prompts, orchestration files and review artifacts under api-conciseness-prompts/. Derive the assignments from the current workspace. Do not execute the previous expansion-oriented prompts unchanged.

The objective is the smallest sufficient documentation for correct use:

- Give each abstraction one concise purpose sentence.
- Include a small, focused example only when it materially improves understanding.
- Retain essential contracts: invariants, ownership, lifetime, ordering, units, failure behavior and other constraints callers need.
- Remove repetition, signature restatements, unnecessary implementation detail and explanations that do not change how callers use the API.
- Keep lengthy demonstrations in appropriate module-owned usage documentation when worth retaining; link them from the header.
- Treat coverage checklists as investigation aids, not requirements to add a paragraph about every topic.
- Preserve existing terminology, public declarations, implementation behavior and unresolved TODOs. Do not invent semantics to shorten an explanation.
- Skip external libraries without user-authored APIs and record unavailable source modules separately.
- Leave already-concise documentation unchanged. Do not impose arbitrary length limits.

Inspect enough implementation and existing validation to confirm that shortening preserves meaning. Check references and documentation structure. Compile materially changed examples where practical; use temporary harnesses rather than adding scaffolding to public comments. Report existing failures separately without changing tests or implementation to make this documentation task pass.

Perform an explicit editorial review after each batch: can a caller understand the API at a glance, and does every retained detail earn its place?

Continue the existing local branch docs/api-documentation-finish in each owning repository:

- /home/gilqamesh/Projects/Builder
- /home/gilqamesh/Projects/Builder-Modules
- /home/gilqamesh/Projects/Builder-Private

These branches were created to finish this documentation pass and already contain the initial API documentation changes. Verify that each repository is on its finish-off branch before editing; do not create replacement branches.

Inventory existing changes before editing. Existing uncommitted API documentation is input to this review and may be included in the final commits once reviewed. Preserve unrelated changes and exclude them from commits; inspect staged diffs carefully, including files containing mixed changes.

Only the coordinating session should switch branches, stage files and commit. After the batches and validation finish, commit the reviewed documentation changes in each repository with concise one-sentence technical messages. Do not manufacture changes or empty commits for a repository needing none. Do not push.

Return concise per-module findings with actual diff links, representative before/after excerpts, validation results and remaining caveats. Finish with each repository's branch name and commit hash.
