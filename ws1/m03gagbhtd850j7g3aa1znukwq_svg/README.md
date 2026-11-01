# SVG rasterization

## Goal

Render an SVG file into a PNG image so tools can consume vector diagrams as raster output. This module owns the conversion invocation and output checks; SVG authoring and the meaning of the resulting image remain with callers.

See [svg.h](svg.h) for the public interface.
