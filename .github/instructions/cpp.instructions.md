---
applyTo: "**/*.h,**/*.cpp"
---

# General C++ Instructions

## Language and API design

- Use C++23.
- Public APIs must be concise, technically precise, and understandable from representative user-authored code.
- Every public type, operation, abstraction, and extension point must own a concrete current semantic responsibility.
- The public surface must be the smallest coherent one that completely expresses the current contract.
- Preserve existing public interfaces and observable semantics unless an authoritative contract or explicit user decision requires a change. For an unresolved durable choice, including one exposed by a private implementation constraint, follow [semantic decision handling](../../docs/agent-workflow.md#settle-semantic-decisions).
- Prefer self-validating values. Successful construction must establish public invariants where practical.
- Group related construction inputs in a typed description when named fields clarify their roles.
- Mark a single-parameter constructor `explicit` unless implicit conversion is intentionally part of the interface.
- Make ownership, borrowing, lifetime, and mutability explicit when they affect public use.
- Prefer immutable construction configuration. Keep immutable descriptions or programs separate from mutable per-use state when they are different concepts.
- Generic and shared abstractions require a concrete shared semantic need; similar implementation vocabulary alone is insufficient.
- Backend-independent abstractions must express backend-independent semantics. Backend mechanics and mismatches belong in backend-private translation or lowering.
- Collaborating types must communicate through ordinary interfaces aligned with ownership boundaries. Structure APIs so `friend`, passkeys, and privileged access shims are unnecessary.
- Future implementation possibilities remain private or undecided until they impose a real semantic requirement.
- Make exception messages identify the failed operation and condition. Include useful actual and expected values when available; construct such messages with `std::format`, passing project-defined values directly so their existing `std::formatter` specializations supply the representation instead of reproducing it at the call site. Use a string literal when the message has no substitutions.

## Module-local helpers

- Establish each capability's owner before choosing helper placement. Extend the responsible abstraction when the requested capability belongs to it.
- Place operations on the type whose responsibility or invariants they implement.
- Consolidate remaining module-level supporting code in one `helpers.h` / `helpers.cpp` pair directly in the module directory.
- Place project helper declarations and definitions directly in the namespace whose name exactly matches the complete module name.
- Apply the [header rules](cpp-headers.instructions.md) and [source rules](cpp-sources.instructions.md) to the pair, including template and formatter placement. Follow the [declaration and definition order](#declaration-and-definition-order) rules.

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

- Store implementation state directly in its owning type.
- Use `std::size_t` for object sizes, element counts, and container indices.
- Use another integer type only when required by an external API, serialized representation, or exact-width constraint.
- Use `std::shared_ptr` for genuinely shared ownership.
- Otherwise select value semantics, references, non-owning raw pointers, or `std::unique_ptr` to express the required ownership and lifetime.

## Formatting

- Use four spaces for indentation.
- Do not use tabs for indentation.
- Place opening braces on the same line as namespace declarations, type declarations, function declarations, and control statements.
- For a constructor with a multiline member-initializer list, place the opening brace on its own line after the final initializer.
- Keep a statement or expression on one line when it remains readable; do not split a simple expression merely to satisfy a fixed line-width target. When an expression is too dense for one line, introduce named intermediate values instead of vertically stacking a simple operator chain.
- When a call or initializer is multiline, place each argument or element on its own line and align the closing delimiter with the start of the construct.
- Prefer `<` and `<=` to `>` and `>=` when reversing the operands preserves the meaning. Express bounded ranges in increasing order, such as `lower <= value && value < upper`.
- Give each non-empty `case` and `default` a braced body, except when its entire body is a single `return` statement; that form may remain unbraced on one line. When a case exits with `break`, place the `break` after the closing brace. Stack adjacent labels only when they intentionally share one body.
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

switch (value) {
    case option: {
        use(value);
    } break;
}
```

## Function interfaces

- Keep single-use private logic in the calling operation.
- For freely mutable properties, provide `T& name()` and `const T& name() const` overloads. Use parameter-taking setters when mutation requires validation or coordinated state changes.
- Declare a member function `const` when it does not modify the object's logical state.
- Pass a non-owning object by `const` reference when the function must not modify it and copying is unnecessary.
- Add reference qualifiers only when lvalue and rvalue invocation require different semantics.
- Add `noexcept` only when every operation executed by the function is guaranteed not to throw.
- Do not add `const`, reference qualifiers, or `noexcept` mechanically.

## Documentation

- Public headers document the non-obvious requirements and guarantees callers need to use the API correctly.
- Do not comment merely to paraphrase a name, type, parameter list, return type, or standard C++ behavior.
- State shared semantics once at the narrowest common scope: type invariants on the type, operation-specific behavior on the operation, and enumerator meaning beside the enumerator.
- When a public declaration needs documentation, use concise Doxygen comments and keep each `@brief` to one sentence.
- Add `@param`, `@return`, precondition, postcondition, ownership, or lifetime documentation only when it adds information.
- Document non-obvious failure conditions when they are part of the public contract, but omit exception-class names and `@throws` tags.
- Place algorithm details, implementation rationale, and maintenance constraints beside their implementation; retain algorithm details in public documentation when callers need them as guarantees.
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
