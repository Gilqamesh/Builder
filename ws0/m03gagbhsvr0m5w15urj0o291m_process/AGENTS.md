# `m03gagbhsvr0m5w15urj0o291m_process`

## Purpose

Represent process commands and own direct execution, checked waits, process replacement, and synchronous foreground-terminal handoff. Shell parsing, pipelines, terminal emulation, asynchronous supervision, and network execution require separate semantic abstractions under the [agent workflow](../../docs/agent-workflow.md#settle-semantic-decisions).

## Invariants

- `command_t` owns its argument strings, optional working directory, and environment additions.
- Foreground handoff is active only when stdin is a terminal; otherwise the foreground API retains checked non-interactive execution semantics.
- An interactive child runs in its own process group. The parent's foreground process group and saved terminal attributes are restored on every recoverable exit path.
- A stopped foreground child is not silently treated as completion; it is terminated and reaped according to the current checked-job contract.

## Validation

For non-interactive foreground-fallback changes, run `test/public_api.cpp` with non-terminal stdin. Terminal handoff, stopped-child handling, and restoration require a pseudo-terminal or manual terminal check and must be reported separately when those behaviors change.
