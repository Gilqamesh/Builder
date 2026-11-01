---
applyTo: "**/*.h,**/*.cpp"
---

# General C++ Instructions

## Language and API design

- This experimental branch uses C++26 with GCC 16 and `-freflection`.
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
- Use the simplest existing operations that satisfy the contract; add specialized handling only for an established requirement.
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
- Use unprefixed `snake_case` for public data members, including fields of module-local helper types.
- Prefix private and protected non-static data members with `m_`. Determine access from the member declaration, including the default access of its enclosing class or struct.
- Use uppercase `SNAKE_CASE` for preprocessor macros and include guards.
- Template parameters may use concise uppercase names such as `T` and `N`.

### Variables named after their types

- Name variables and data members after their types, omitting `_t`. Apply this rule equally to explicit types, `auto`, references, and pointers.
- When multiple instances of a type or template family need differentiation in the same scope, use names that identify their semantic roles. Include the type name or meaningful template arguments when they clarify those roles.
- Use semantic names for scalar and generic storage types whose type names do not identify a useful domain concept.
- Do not distinguish variables with numeric suffixes or generic names such as `value`, `object`, or `instance`.

```cpp
window_t window;
auto input_state = window.input_state(); // Returns input_state_t.

input_state_t previous_input_state;
input_state_t current_input_state;

struct window_description_t {
    std::size_t width;
    std::size_t height;
};

class application_t {
private:
    window_t m_window;
};
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

Write for experienced C++ developers and make the API's purpose and use clear at a glance.

- Explain what problem an abstraction solves and when someone needs it.
- Describe concrete behavior in familiar, precise language. Use as much context as necessary to make the purpose clear, usually one or two sentences.
- Add ownership, lifetime, side effects, and constraints where they affect correct use.
- Omit comments that merely repeat the declaration.
- Use examples when they clarify meaningful usage; keep substantial examples in the module README.
- Use brief implementation comments to explain reasoning needed for maintenance.
- Apply the [module documentation invariant](../../docs/repository-model.md#module-documentation) to public and private comments and examples. State preconditions, ownership, and lifetime requirements directly on values and operations; keep examples within the documented API and its necessary inputs.

Public headers explain abstraction purpose and local, operation-specific semantics. Every module README has one meaningful Goal describing the capability, why someone needs it, and its responsibility boundary, following the [module README convention](../../docs/module-readme.md). READMEs also explain useful shared public models, detailed requirements, and substantial examples. They describe current intent; update the Goal when the module's purpose changes. Keep workflow and coding instructions in Builder's shared documents. Keep each detailed rule with its owner and use brief local summaries where they help correct use. Keep documentation navigation in Markdown.

## Validation design

- Test observable public contracts, public invariants, and relevant negative cases through public APIs.
- Keep tests independent of replaceable implementation details, including backend choice and internal dependency structure.
- Successful-construction tests must show that invalid states are rejected when construction owns that invariant.
- Compile representative caller code that uses public interfaces and extension points.
- Compile and run runnable documentation examples, checking that their results match the documented behavior.

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
