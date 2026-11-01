# DOT rendering

## Goal

Render a supplied DOT graph into an SVG image so tools can turn graph descriptions into viewable diagrams. This module owns the rendering invocation and output path checks; callers own graph content, dependency semantics, and diagram interpretation.

See [dot.h](dot.h) for the public interface.
