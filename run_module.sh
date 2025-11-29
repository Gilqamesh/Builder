#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 ]]; then
    echo "Usage: $0 <module> [-- zig-args]" >&2
    exit 1
fi

MODULE="$1"
shift || true

ROOT="$(cd "$(dirname "$0")" && pwd)"
MODULE_DIR="${ROOT}/src/${MODULE}"

if [[ ! -d "$MODULE_DIR" ]]; then
    echo "Unknown module: ${MODULE}" >&2
    exit 1
fi

if [[ ! -f "${MODULE_DIR}/build.zig" ]]; then
    echo "Module ${MODULE} is missing a build.zig" >&2
    exit 1
fi

cd "$MODULE_DIR"

if grep -Fq 'step\("run"' build.zig; then
    exec zig build run "$@"
elif grep -Fq 'step\("test"' build.zig; then
    exec zig build test "$@"
else
    echo "No explicit run or test step found; running plain build." >&2
    exec zig build "$@"
fi
