# SHA-256 generation and verification

## Goal

Generate portable SHA-256 checksum files and verify files against supplied
expected digests. Check file contents without acquiring files or establishing
trust in expected digests.

See [sha256sum.h](sha256sum.h) for the public interface.

## Checksum files

`generate(path)` creates the adjacent `path.sha256` and returns its path. The
input must be a regular file and the destination must not exist. The input is
unchanged. The checksum uses GNU format and records the input's basename
(`./-` for a file named `-`), so both files can be moved together and checked
from their directory:

```sh
sha256sum -c archive.tar.gz.sha256
```

`verify(path, expected_sha256)` retains its expected-digest interface and uses
the adjacent `.sha256` path temporarily; that path must be absent. Use the
checksum-file command above for a retained generated sidecar.

## Validation

`test/public_api.cpp` covers expected-digest verification, checksum generation,
portable filename records, and protection of existing checksum files.
