# Builder CLI

## Goal

Build and run a selected module binary target so developers and tools can invoke module programs directly from a workspace. The CLI owns argument routing, bootstrap self-update, and process launch or replacement; build phases own output construction and the target module owns application behavior.

See [builder_cli.h](builder_cli.h) for the public interface.
