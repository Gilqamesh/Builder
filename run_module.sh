#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 ]]; then
    echo "Usage: $0 <module-name|//label> [-- <args...>]" >&2
    exit 1
fi

target="$1"
shift || true

if [[ "$target" != //:* && "$target" != //src/*:* && "$target" != //*:* ]]; then
    target="//src/${target}:${target}"
fi

if ! bazel query "kind(cc_binary, ${target})" >/dev/null 2>&1; then
    echo "No runnable cc_binary target found for ${target}" >&2
    exit 1
fi

bazel run "${target}" -- "$@"
