#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(cd -- "$SCRIPT_DIR/.." && pwd)
BUILD_DIR=${BUILD_DIR:-"$REPO_ROOT/build/tests"}

cmake -S "$REPO_ROOT/programs" -B "$BUILD_DIR" -DPROGRAM_FILE="$REPO_ROOT/programs/tests.cmake"
cmake --build "$BUILD_DIR" --target builder_tests
ctest --test-dir "$BUILD_DIR"
