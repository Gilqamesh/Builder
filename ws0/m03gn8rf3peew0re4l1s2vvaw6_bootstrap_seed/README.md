# Bootstrap seed

## Goal

Define the modules needed to bootstrap Builder and expose their newest source version so the CLI can determine when its seed must be rebuilt. This module owns seed identity and membership; compilation, dependency execution, and transfer to the rebuilt CLI belong to their foundation owners.

See [bootstrap_seed.h](bootstrap_seed.h) for the public interface.
