# Signal handling

## Goal

Support orderly shutdown around synchronous work by recording termination requests and forwarding termination signals to a forked child. Provide scoped interruption handling without executing commands, managing foreground terminals, or performing recovery.

See [signal_handler.h](signal_handler.h) for the public interface.
