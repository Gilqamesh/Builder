# Source dependencies

## Goal

Derive module dependencies from source includes and recursively visited local headers so builds can select eligible dependencies and their closure. The module owns source-set scanning and workspace dependency rules; workspace discovery, build scheduling, and artifact publication belong to the other foundation modules.

See [source_dependencies.h](source_dependencies.h) for the public interface.
