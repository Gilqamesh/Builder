# `m03gagbhsvr0m5w15urj0o291m_process`

## Purpose

Represent process commands, replace or create child processes, report checked results, and safely manage terminal foreground ownership for interactive child jobs.

## Invariants

- `command_t` owns its argument strings, optional working directory, and environment additions.
- A normal wait returns a non-negative exit status or the negated terminating signal number; checked operations throw on non-zero results.
- Child failure paths terminate with `_exit` and never return into parent-only control flow.
- Interrupted system calls that are expected to be retryable are retried on `EINTR`.
- Error reporting preserves the error from the failed operation across cleanup where necessary.
- Foreground handling is active only when stdin is an applicable terminal; non-interactive execution must continue to work.
- The child enters its own process group before execution, and the handoff gate prevents it from racing terminal use before the parent transfers foreground ownership.
- `SIGTTOU` is blocked only around foreground-process-group changes and the previous signal mask is restored.
- The parent's foreground process group and saved terminal attributes are restored on every recoverable exit path and again without throwing during destruction.
- A stopped foreground child is not silently treated as completion; it is terminated and reaped according to the current checked-job contract.
- File descriptors and child processes are not leaked on partial setup failure.

## Boundary

This module owns direct command execution, checked waits, process replacement, and synchronous foreground-terminal handoff. Shell parsing, pipelines, terminal emulation, asynchronous supervision, and network execution require separate semantic abstractions.

## Validation

Exercise:

1. successful and non-zero child exits;
2. termination by signal;
3. invalid executable and working-directory failures;
4. non-interactive execution with redirected stdin;
5. interactive execution in a pseudo-terminal, including terminal restoration after success and failure;
6. termination during foreground setup and waiting.

Report pseudo-terminal automation and manual terminal checks separately.

## Direction required

Obtain direction before changing result encoding, environment inheritance, signal forwarding, process-group ownership, stopped-child behavior, terminal restoration, or checked-operation semantics.
