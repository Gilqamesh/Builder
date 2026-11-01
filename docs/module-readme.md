# Module README convention

Every module has a `README.md` with exactly one meaningful `## Goal` section.
Describe the capability provided, why someone needs it, and the responsibility
boundary. A short paragraph is sufficient; a directory-name paraphrase or a
placeholder is not a Goal. Describe the capability without fixing replaceable
dependencies or internal algorithms. Apply the [module documentation
invariant](repository-model.md#module-documentation) to every section, example,
and link. State requirements on supplied values, operations, and storage directly;
keep examples within the documented API and its necessary inputs.

The README describes current intent. Establish its Goal from the task's settled
intent before designing a new module's public API, write the README as part of
completing module creation, and change the Goal when the module's purpose changes.
The module generator supplies neutral scaffolding; the authoring workflow supplies
the Goal. See [module creation](agent-workflow.md#create-a-new-module).

## Additional content

After the module heading and Goal, add sections only when they help someone use
or change the module correctly. Useful subjects include:

- Model: shared concepts, ownership, relationships, invariants, and detailed requirements.
- Usage: self-contained API examples and substantial workflows.
- Limitations: unsupported behavior, responsibility boundaries, and explicitly unresolved decisions.
- Validation: useful module-specific entry points, environments, or manual checks.

Use descriptive headings when they make the material clearer. There is no required
section checklist beyond Goal. Preserve settled decisions and guarantees; keep
unresolved choices explicitly unresolved until direction is established. Consolidate
overlapping explanations rather than copying the same detailed rule into several
places.

## Documentation ownership

| Material | Owner |
|---|---|
| Module capability, motivation, and responsibility boundary | README Goal. |
| Shared public concepts, detailed requirements, invariants, limitations, and substantial examples | Module README. |
| Public abstraction purpose and operation-specific semantics, including constraints needed at the call site | Public headers. |
| Useful module-specific validation entry points and environment requirements | Module README validation section. |
| Shared workflow, coding, comment, and validation rules | Builder's [agent workflow](agent-workflow.md) and matching files under `.github/instructions/`. |
| Cross-module and build-system semantics | [Repository model](repository-model.md). |
| Dependency declarations | The source graph, as defined by [dependency discovery](repository-model.md#dependency-discovery). |
| Private implementation rationale and storage maintenance | Beside the code they govern. |
| Executable evidence and test-case inventories | Tests. |
| Task plans, obtained results, caller or data migration steps, and historical alternatives | The [logical change record](agent-workflow.md#logical-change-record) and its completion reporting rules. |

Keep workflow and coding instructions centralized in Builder. Module READMEs supply
the semantics needed to apply them, including for modules in Builder-Modules and
other repositories in the combined workspace. Keep temporary working notes and
transcripts outside permanent documentation.

Preserve the [C++ documentation split](../.github/instructions/cpp.instructions.md#documentation):
headers explain abstraction purpose and local operation semantics; READMEs explain
shared concepts and detailed requirements. Keep each detailed rule with its owner,
use brief local summaries when correct use requires them, and keep documentation
navigation in Markdown.
