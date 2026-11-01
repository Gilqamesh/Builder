# SHA-256 verification

## Goal

Verify a file against a supplied expected SHA-256 digest and reject unexpected contents. Check the digest comparison without acquiring the file or establishing trust in the expected digest.

See [sha256sum.h](sha256sum.h) for the public interface.
