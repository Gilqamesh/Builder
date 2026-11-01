# `m03gtrxnmqqa2t7zxpijo222n6_formatting`

## Goal

Provide structural formatting through inheritance by explicit `std::formatter` specializations. Produce consistent diagnostic views of structured values through shared member discovery, recursive layout, and shortening; custom formatters can provide semantic presentation.

See [api.h](api.h) for formatter selection and layout options.

## Invariants

Selecting structural formatting exposes the type's public, protected, and private
subobjects. Access control must not filter the structural representation. Unsupported
structure requires a custom formatter. Formatting borrows const values and does not
traverse pointer ownership graphs.

## Boundaries

Formatting owns member discovery, recursive layout, and value shortening. The
structural representation does not infer invariants, units, semantic names,
resource summaries, or derived metrics; those require a custom formatter.
Structural diagnostics are not a serialization or persistence format.

## Example

Define the value and select structural formatting through an explicit specialization:

```cpp
#include <m03gtrxnmqqa2t7zxpijo222n6_formatting/api.h>

#include <format>
#include <iostream>

struct point_t {
    int x;
    int y;
};

namespace std {
template <>
struct formatter<point_t> : m03gtrxnmqqa2t7zxpijo222n6_formatting::reflected_formatter_t {};
} // namespace std

int main() {
    const point_t point{1, 2};
    std::cout << std::format("{}\n", point);   // Expanded.
    std::cout << std::format("{:1}\n", point); // Inline leaf records.
    std::cout << std::format("{:2}\n", point); // Single line.
    std::cout << std::format("{:3}\n", point); // Shortened single line.
    return std::format("{:2}", point) == "point_t { x: 1, y: 2 }" ? 0 : 1;
}
```
