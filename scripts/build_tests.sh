#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(cd -- "$SCRIPT_DIR/.." && pwd)
BUILD_DIR=${BUILD_DIR:-"$REPO_ROOT/build/tests"}

cmake -S "$REPO_ROOT/src/tests_main" -B "$BUILD_DIR"
cmake --build "$BUILD_DIR" --target tests
