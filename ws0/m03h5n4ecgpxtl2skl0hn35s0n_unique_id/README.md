# Shared unique identities

## Goal

Provide canonical UUIDv7 identities with a fixed-width base36 representation, generation, validation, comparison, and formatting. Keep identity representation independent of domain-specific prefixes, suffixes, naming rules, and lifecycle policies.

## Model and requirements

`unique_id_t` owns a 128-bit UUIDv7. [The public API](unique_id.h) specifies
construction and formatting.

The representation is exactly 25 lowercase base36 digits (`0-9`, `a-z`), treating
the UUID's network-order bytes as an unsigned integer and preserving leading zeros.
Parsing rejects other widths, case, whitespace, prefixes, invalid UUID versions or
variants, and values exceeding 128 bits. There is no empty or nil identity.
Equality compares UUID values. Construction copies its inputs; the UUID accessor
returns a value, and no mutable view of the stored identity is exposed.

Generation delegates to the existing UUIDv7 generator, including its ordering and
failure semantics. Identity timestamps do not establish a version-history order.
The UUID module retains its standard hexadecimal formatting and CLI behavior;
formatting a `unique_id_t` always produces the canonical base36 representation
and accepts only an empty format specification.

The dependencies are UUID generation and validation from the UUID module and
general radix conversion from the byte-stream module.

## Example

```cpp
#include <m03h5n4ecgpxtl2skl0hn35s0n_unique_id/unique_id.h>

int main() {
    using m03h5n4ecgpxtl2skl0hn35s0n_unique_id::unique_id_t;

    const auto generated_id = unique_id_t::generate();
    const auto text = generated_id.string();
    const unique_id_t parsed_id(text);
    return parsed_id == generated_id ? 0 : 1;
}
```

The round-trip example is exercised in `test/public_api.cpp`.
