#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 ]]; then
    echo "Usage: $0 <module|//label> [-- args]" >&2
    exit 1
fi

ROOT="$(pwd)"
WORK="${ROOT}/.bazel_work"
mkdir -p "$WORK"

# Ensure Bazel sees real project sources
if [[ ! -e "${WORK}/src" ]]; then
    ln -s "${ROOT}/src" "${WORK}/src"
fi

# Disable ccache for Bazel, correctly
echo "build --action_env=CCACHE_DISABLE=1" > "${WORK}/.bazelrc"
echo "build --repo_env=CCACHE_DISABLE=1" >> "${WORK}/.bazelrc"

# Regenerate MODULE.bazel
cat > "${WORK}/MODULE.bazel" <<EOF
module(name = "builder_workspace")

# Load raylib module if present
raylib_ext = use_extension("//src/raylib:extension.bzl", "raylib_ext")
use_repo(raylib_ext, "raylib")
EOF

# Normalize target name
target="$1"
shift || true

if [[ "$target" != //:* ]]; then
    target="//src/${target}:${target}"
fi

cd "$WORK"
bazelisk run "$target" -- "$@"
