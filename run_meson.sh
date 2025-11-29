#!/bin/sh
set -e

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <module-name>" >&2
    exit 1
fi

module="$1"
cd "src/$module"

build_dir="${BUILD_DIR:-build}"

if [ ! -d "$build_dir" ]; then
    meson setup "$build_dir"
fi

meson compile -C "$build_dir"
meson test -C "$build_dir" --no-rebuild || true
