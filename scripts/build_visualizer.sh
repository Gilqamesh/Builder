#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(cd -- "$SCRIPT_DIR/.." && pwd)
BUILD_DIR=${BUILD_DIR:-"$REPO_ROOT/build/visualizer"}

cmake -S "$REPO_ROOT/programs/visualizer" -B "$BUILD_DIR"
cmake --build "$BUILD_DIR" --target visualizer
