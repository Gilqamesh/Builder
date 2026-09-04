---
applyTo: "**/*.cpp"
---

# C++ Source Instructions

## Includes

- Include the source file's corresponding header first.
- After the corresponding header, include any additional headers from the same source directory.
- Place headers exposed by other repository modules in a second group.
- Separate the two non-empty groups with exactly one blank line.
- Do not insert blank lines within a group.
- Do not repeat declarations from the corresponding header to avoid including it.

```cpp
# include "builder_cli.h"
# include "internal_helpers.h"

# include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>
```

## Namespace and definitions

- Define project functions in the namespace whose name exactly matches the complete module name.
- Close every named namespace with a comment containing its complete name.
- Define functions in exactly the same order as their declarations in the corresponding header.
- Preserve the relative order of overloads.
- Do not regroup constructors, operators, accessors, helpers, or other functions by implementation category.
- Do not introduce a duplicate declaration before an out-of-class definition.

```cpp
namespace complete_module_name {

foo_t::foo_t() {
}

void foo_t::value(int value) {
    m_value = value;
}

int foo_t::value() const {
    return m_value;
}

} // namespace complete_module_name
```

## Header-owned definitions

- Keep template definitions in the header that declares the template.
- Keep `std::formatter` declarations and definitions in the header that introduces the formatted type.
- Do not move template or formatter definitions into a source file unless the current task explicitly requires a different instantiation strategy.
