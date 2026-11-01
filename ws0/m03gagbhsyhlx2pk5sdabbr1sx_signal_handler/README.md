# Signal handling

## Goal

Support orderly shutdown around synchronous work by recording termination requests and forwarding termination signals to a forked child. This gives build and process code a scoped way to respond to interruption; command execution, foreground terminal handoff, and application recovery policy belong to their respective consumers.

See [signal_handler.h](signal_handler.h) for the public interface.
