#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(cd -- "$SCRIPT_DIR/.." && pwd)
BUILD_DIR=${BUILD_DIR:-"$REPO_ROOT/build/function_visualizer"}

mkdir -p "$BUILD_DIR"

cat > "$BUILD_DIR/CMakeLists.txt" <<EOF
cmake_minimum_required(VERSION 3.14)
project(function_visualizer)
include("$REPO_ROOT/src/function_visualizer.cmake")
EOF

cmake -S "$BUILD_DIR" -B "$BUILD_DIR"
cmake --build "$BUILD_DIR" --target function_visualizer
