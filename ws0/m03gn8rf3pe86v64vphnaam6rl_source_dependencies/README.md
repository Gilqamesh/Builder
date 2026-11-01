# Source dependencies

## Goal

Derive module dependencies from source includes and recursively visited local headers. Apply workspace dependency-eligibility rules and compute dependency closure over the discovered source graph.

See [source_dependencies.h](source_dependencies.h) for the public interface.
