---
applyTo: "**/*.h,**/*.cpp"
---

# General C++ Instructions

## Language and API design

- Use C++23.
- Public APIs must be concise, technically precise, and understandable from representative user-authored code.
- Every public type, operation, abstraction, and extension point must own a concrete current semantic responsibility.
- The public surface must be the smallest coherent one that completely expresses the current contract.
- Preserve existing public interfaces and observable semantics unless an authoritative contract or explicit user decision requires a change. A broad request to audit, harden, correct, or reconcile does not itself settle a new public contract.
- Prefer self-validating values. Successful construction must establish public invariants where practical.
- Mark a single-parameter constructor `explicit` unless implicit conversion is intentionally part of the interface.
- Make ownership, borrowing, lifetime, and mutability explicit when they affect public use.
- Keep immutable descriptions or programs separate from mutable per-use state when they are different concepts.
- Generic and shared abstractions require a concrete shared semantic need; similar implementation vocabulary alone is insufficient.
- Backend-independent abstractions must express backend-independent semantics. Backend mechanics and mismatches belong in backend-private translation or lowering.
- Collaborating types must communicate through ordinary interfaces aligned with ownership boundaries. Structure APIs so `friend`, passkeys, and privileged access shims are unnecessary.
- Treat a private implementation constraint that would affect the public model as an unresolved semantic constraint unless an authoritative contract or explicit user decision settles it.
- Future implementation possibilities remain private or undecided until they impose a real semantic requirement.

## Naming

- Names must be technically precise and as short as their context permits.
- Let the module namespace and enclosing type carry context; within a renderer module, for example, prefer `program_t` to `renderer_program_t`.
- Use `snake_case` for project-defined namespaces, functions, variables, data members, and enumerators.
- Suffix project-defined class, struct, enum, and type-alias names with `_t`.
- Prefix every non-static data member with `m_`.
- Use uppercase `SNAKE_CASE` for preprocessor macros and include guards.
- Template parameters may use concise uppercase names such as `T` and `N`.

### Variables named after their types

For a variable of type `foo_t`:

- Prefer `foo` when there is one such variable and its role is unambiguous.
- Use a descriptive role-based name when multiple variables have that type or when the variable has a more specific role.
- Do not distinguish variables with numeric suffixes or generic names such as `value`, `object`, or `instance`.

```cpp
window_t window;

input_state_t previous_state;
input_state_t current_state;
```

## Types and ownership

- Use `std::size_t` for object sizes, element counts, and container indices.
- Use another integer type only when required by an external API, serialized representation, or exact-width constraint.
- Use `std::shared_ptr` for genuinely shared ownership.
- Otherwise select value semantics, references, non-owning raw pointers, or `std::unique_ptr` to express the required ownership and lifetime.

## Formatting

- Use four spaces for indentation.
- Do not use tabs for indentation.
- Place opening braces on the same line as namespace declarations, type declarations, function declarations, and control statements.
- For a constructor with a multiline member-initializer list, place the opening brace on its own line after the final initializer.
- Preserve surrounding formatting for constructs not specified here.

```cpp
void function() {
    if (condition) {
        // ...
    }
}

foo_t::foo_t():
    m_value(0)
{
}
```

## Function interfaces

- Declare a member function `const` when it does not modify the object's logical state.
- Pass a non-owning object by `const` reference when the function must not modify it and copying is unnecessary.
- Add reference qualifiers only when lvalue and rvalue invocation require different semantics.
- Add `noexcept` only when every operation executed by the function is guaranteed not to throw.
- Do not add `const`, reference qualifiers, or `noexcept` mechanically.

## Documentation

- Public headers document observable contracts that are not evident from a declaration and its enclosing documentation.
- Do not comment merely to paraphrase a name, type, parameter list, return type, or standard C++ behavior.
- State shared semantics once at the narrowest common scope: type invariants on the type, operation-specific behavior on the operation, and enumerator meaning beside the enumerator.
- When a public declaration needs documentation, use concise Doxygen comments and keep each `@brief` to one sentence.
- Add `@param`, `@return`, precondition, postcondition, ownership, or lifetime documentation only when it adds information.
- Document non-obvious failure conditions when they are part of the public contract, but omit exception-class names and `@throws` tags.
- Keep implementation rationale and private maintenance constraints beside the code they govern; public documentation describes observable behavior.
- Preserve documented semantics. Consolidate or remove prose when it is redundant, incorrect, or obsolete.

## Validation design

- Tests must demonstrate observable contracts, public invariants, and relevant negative cases.
- Successful-construction tests must show that invalid states are rejected when construction owns that invariant.
- Backend-neutral contract tests must remain independent of backend-private mechanics.

## Declaration and definition order

- Treat declaration order as canonical.
- Write definitions in exactly the same order as their corresponding declarations.
- Preserve the relative order of overloads.
- Apply this rule to constructors, destructors, conversion functions, operators, member functions, and free functions.
- Do not regroup definitions by implementation category.

## Includes

- Include every dependency directly rather than relying on unrelated transitive includes.
- Include only the headers required by the file.
- Standard-library headers may be included directly.
- System, platform, and third-party headers may be included directly unless an existing repository module intentionally owns the required abstraction.
- Use a repository module's public interface when that module owns the required abstraction.
- Use complete module-qualified paths for repository-module headers.
- Keep include paths independent of filesystem traversal.
- Follow the path-specific directive spelling: headers use `# include`; source files use `#include`.

For example, a source-file include block uses:

```cpp
#include "same_directory_header.h"

#include <complete_module_name/header.h>

#include <vector>
```
