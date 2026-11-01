# Artifact storage

## Goal

Locate versioned build artifacts, recognize completed outputs, and atomically update latest references so build phases can reuse and publish results consistently. This module owns artifact paths, marker paths, completed-output lookup, and latest-link operations; phase execution and the decision that an output is ready to publish belong to build phases.

See [artifact_store.h](artifact_store.h) for the public interface.
