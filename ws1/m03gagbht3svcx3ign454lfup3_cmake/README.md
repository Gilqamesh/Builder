# CMake integration

## Goal

Configure, build, and install CMake projects from module producer code so external projects can participate in Builder builds. This module supplies checked CMake invocations with explicit directories and settings; producers own source acquisition, project-specific options, and publication of the resulting artifacts.

See [cmake.h](cmake.h) for the public interface.
