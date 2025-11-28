#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(cd -- "$SCRIPT_DIR/.." && pwd)
BUILD_DIR=${BUILD_DIR:-"$REPO_ROOT/build/tests"}

mkdir -p "$BUILD_DIR"

cat > "$BUILD_DIR/CMakeLists.txt" <<EOF
cmake_minimum_required(VERSION 3.14)
project(tests)
include("$REPO_ROOT/src/tests_main.cmake")
EOF

cmake -S "$BUILD_DIR" -B "$BUILD_DIR"
cmake --build "$BUILD_DIR" --target tests
