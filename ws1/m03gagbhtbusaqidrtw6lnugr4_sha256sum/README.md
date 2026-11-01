# SHA-256 verification

## Goal

Verify a file against an expected SHA-256 digest so consumers can reject unexpected file contents. This module owns the checked digest comparison; callers supply the trusted expected digest and decide how files are acquired and used.

See [sha256sum.h](sha256sum.h) for the public interface.
