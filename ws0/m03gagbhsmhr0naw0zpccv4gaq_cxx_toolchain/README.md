# C++ toolchain

## Goal

Compile module source sets and link libraries or executables from explicit include paths, definitions, and ordered link inputs. Build phases and producer plugins use this capability to construct native outputs; dependency discovery, phase scheduling, and artifact publication remain with their respective build-system owners.

See [cxx_toolchain.h](cxx_toolchain.h) for the public interface.
