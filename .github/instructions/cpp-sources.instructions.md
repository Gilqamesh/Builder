---
applyTo: "**/*.cpp"
---

# C++ Source Instructions

## Includes

Organize includes into these groups:

1. Same-directory headers, using quotes and placing the source file's corresponding header first.
2. Repository-module headers, using complete module-qualified angle-bracket paths.
3. Standard-library, system/platform, and third-party headers.

Separate adjacent non-empty groups with exactly one blank line.
Do not insert blank lines within a group.
Do not repeat declarations from the corresponding header to avoid including it.
Use `#include` without a space between `#` and `include`.

```cpp
#include "builder_cli.h"
#include "helpers.h"

#include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>

#include <cstddef>
#include <string>
```

## Namespace and definitions

- Define project functions in the namespace whose name exactly matches the complete module name.
- Close every named namespace with a comment containing its complete name.
- Follow the [declaration and definition order](cpp.instructions.md#declaration-and-definition-order) rules.
- Do not introduce a duplicate declaration before an out-of-class definition.

```cpp
namespace complete_module_name {

foo_t::foo_t() {
}

int& foo_t::value() {
    return m_value;
}

const int& foo_t::value() const {
    return m_value;
}

} // namespace complete_module_name
```

## Header-owned definitions

Keep template definitions in the declaring header, and `std::formatter` declarations and definitions in the header introducing the formatted type. Move template or formatter definitions into a source file only when the current task explicitly requires a different instantiation strategy.
