---
applyTo: "**/*.h"
---

# C++ Header Instructions

## Required file order

A header must contain sections in this order:

1. Include guard opening.
2. Includes.
3. Module namespace containing project declarations.
4. `std` namespace containing `std::formatter` declarations, when required.
5. Reopened module namespace containing template definitions, when required.
6. Reopened `std` namespace containing `std::formatter` definitions, when required.
7. Include guard closing.

Omit empty sections and their placeholder comments; keep declaration and definition sections separate.

## Include guards

Every `.h` file must begin with a conventional include guard.
Do not place comments, includes, declarations, or other content before it.
Do not use `#pragma once`.

Construct the guard as:

```text
UPPERCASE_MODULE_NAME_UPPERCASE_FILE_BASENAME_H
```

Procedure:

1. Take the complete module directory name.
2. Convert it to uppercase.
3. Append `_`.
4. Append the header base name without `.h`, converted to uppercase.
5. Append `_H`.
6. Replace any non-alphanumeric character with `_`.

For module `m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer` and file `abcd.h`:

```cpp
#ifndef M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_ABCD_H
# define M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_ABCD_H

// Header contents

#endif // M03GL8A1HL8XE3YNM8S2WWFY4U_SOFTWARE_RENDERER_ABCD_H
```

Additional rules:

- Preserve intentional repetition between the module name and file name.
- Use the file base name, not its filesystem path.
- Keep `#ifndef` and `#endif` flush with `#`.
- Place one space between `#` and `define`.
- End `#endif` with a comment containing the complete guard.
- End the file with exactly one newline.

## Includes

A header must not include itself.

Organize includes into these groups:

1. Same-directory headers, using quotes.
2. Repository-module headers, using complete module-qualified angle-bracket paths.
3. Standard-library, system/platform, and third-party headers.

Separate adjacent non-empty groups with exactly one blank line.
Do not insert blank lines within a group.

```cpp
# include "mesh.h"
# include "material.h"

# include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>
# include <m03ginwy24ng8o487c4beoms6l_vector/api.h>

# include <cstddef>
# include <string>
# include <vector>
```

## Project declarations

- Open the namespace whose name exactly matches the complete module name after the include section.
- Place project declarations in that namespace.
- Keep `friend` declarations, passkeys, privileged access shims, and equivalent hidden-access mechanisms out of project types.
- Close every named namespace with a comment containing its complete name.

```cpp
namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer {

// Project declarations

} // namespace m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer
```

## Definitions permitted in headers

Place ordinary non-template project function definitions in the corresponding source file. Header definitions are permitted only for:

- Template definitions that must be visible at the point of instantiation.
- `std::formatter` specialization definitions.
- Definitions explicitly required in the header by the current task or by a language constraint.

Keep member-function definitions outside project class or struct declarations in the module namespace unless the current task or language specifically requires an in-class definition. `inline` alone does not permit moving an implementation into a header.

## Template organization

Declare complete project template types and all members in the initial module-namespace section. Place template definitions in the reopened module namespace shown in [Required file order](#required-file-order), following the shared [declaration and definition order](cpp.instructions.md#declaration-and-definition-order). Preserve the relative order of multiple template types.

## `std::formatter` specializations

Provide a `std::formatter` specialization for every project-defined class, struct, or enum whose primary definition is introduced by the header.

Do not create a separate formatter specialization for:

- A type that is only referenced or forward-declared.
- A type defined by another module.
- A type alias, because an alias does not introduce a distinct type.

Formatter rules:

- Use the `std` sections in [Required file order](#required-file-order) for formatter declarations and definitions.
- Refer to the project type by its fully qualified module name.
- Keep formatter declarations and definitions in the same relative order as their corresponding project types.
- In each `format()` function, obtain `auto out = ctx.out()`, progressively update it with `out = std::format_to(out, ...)`, and finish with exactly one normal `return out;`.
- Make each non-error branch update `out` and continue to the final return; throwing branches may exit independently.
- Build compound output through progressive `out` assignments rather than one aggregate formatting call.
- Treat parsing and the text emitted for each type as separate semantic decisions.
- Do not add anything else to `namespace std` except permitted standard-library specializations.

## Canonical template header layout

```cpp
#ifndef COMPLETE_MODULE_NAME_VALUE_H
# define COMPLETE_MODULE_NAME_VALUE_H

# include "local_dependency.h"

# include <complete_dependency_module/header.h>

# include <format>

namespace complete_module_name {

template <typename T>
class value_t {
public:
    value_t();

    const T& value() const;

private:
    T m_value;
};

} // namespace complete_module_name

namespace std {

template <typename T>
struct formatter<complete_module_name::value_t<T>>;

} // namespace std

namespace complete_module_name {

template <typename T>
value_t<T>::value_t():
    m_value()
{
}

template <typename T>
const T& value_t<T>::value() const {
    return m_value;
}

} // namespace complete_module_name

namespace std {

template <typename T>
struct formatter<complete_module_name::value_t<T>> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const complete_module_name::value_t<T>& value, auto& ctx) const {
        auto out = ctx.out();

        out = std::format_to(out, "{{ ");
        out = std::format_to(out, "value: {}", value.value());
        out = std::format_to(out, " }}");

        return out;
    }
};

} // namespace std

#endif // COMPLETE_MODULE_NAME_VALUE_H
```
