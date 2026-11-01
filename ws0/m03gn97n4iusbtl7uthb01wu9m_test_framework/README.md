# Test framework

## Goal

Provide assertions, exception checks, and a test runner for small native C++ validation programs. Module authors use it to express observable contract checks and receive useful failure reports; each module owns its test cases and the build system owns when registered validation runs.

See [test_framework.h](test_framework.h) for the public interface.
