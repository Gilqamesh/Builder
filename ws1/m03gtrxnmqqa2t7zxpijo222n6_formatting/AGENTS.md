# `m03gtrxnmqqa2t7zxpijo222n6_formatting`

## Purpose

Own the shared structural implementation inherited by explicit `std::formatter`
specializations. Type owners select structural formatting in their own headers
and retain custom formatters for semantic presentation.

## Invariants

Reflection describes public value structure. It must not silently omit inaccessible
subobjects or inspect private resource storage. Unsupported structure requires a
custom formatter. Formatting borrows const values and does not traverse pointer
ownership graphs.

## Boundaries

Formatting owns member discovery, recursive layout, and value shortening. Type owners
own invariants, units, semantic names, resource summaries, and derived metrics.
Structural diagnostics are not a serialization or persistence format.
