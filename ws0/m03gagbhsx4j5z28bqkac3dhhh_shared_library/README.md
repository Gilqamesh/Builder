# Shared library loading

## Goal

Load a shared library and resolve named function symbols so hosts can invoke separately built plugins. The module owns the loaded library lifetime and symbol lookup; callers own the expected function signature, plugin ABI, and use of resolved symbols while the library remains loaded.

See [shared_library.h](shared_library.h) for the public interface.
