# Verified downloads

## Goal

Fetch a URL into a file and verify it against an expected checksum so module producers can consume pinned external sources. This module owns the download and integrity check; producers choose the source and trusted digest and own extraction and publication.

See [download.h](download.h) for the public interface.
