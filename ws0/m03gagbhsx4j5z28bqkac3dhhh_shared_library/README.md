# Shared library loading

## Goal

Load a shared library, own its loaded lifetime, and resolve named function symbols. The requested function signature must match the resolved symbol and its ABI; resolved symbols may be invoked only while the library remains loaded.

See [shared_library.h](shared_library.h) for the public interface.
