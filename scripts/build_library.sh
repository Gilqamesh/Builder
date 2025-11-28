#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(cd -- "$SCRIPT_DIR/.." && pwd)
BUILD_DIR=${BUILD_DIR:-"$REPO_ROOT/build/library"}

cmake -S "$REPO_ROOT/programs/library" -B "$BUILD_DIR"
cmake --build "$BUILD_DIR" --target builder_lib
