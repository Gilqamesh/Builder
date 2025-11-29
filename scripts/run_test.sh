#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 ]]; then
    echo "Usage: $0 <target> [bazel args...]" >&2
    echo "Example: $0 //call:call_tests" >&2
    exit 1
fi

SCRIPT_DIR=$(cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(cd -- "$SCRIPT_DIR/.." && pwd)
WORKSPACE_DIR=${WORKSPACE_DIR:-"$REPO_ROOT/.bazel_workspace"}
BAZEL_BIN=${BAZEL_BIN:-bazel}

mkdir -p "$WORKSPACE_DIR"

cat > "$WORKSPACE_DIR/WORKSPACE.bazel" <<EOF_WORKSPACE
workspace(name = "builder_workspace")

local_repository(
    name = "builder",
    path = "$REPO_ROOT/src",
)
EOF_WORKSPACE

touch "$WORKSPACE_DIR/BUILD.bazel"

RAW_TARGET=$1
shift || true

case "$RAW_TARGET" in
    @*) TARGET_LABEL="$RAW_TARGET" ;;
    //*) TARGET_LABEL="@builder${RAW_TARGET}" ;;
    *) TARGET_LABEL="@builder//${RAW_TARGET}" ;;
esac

pushd "$WORKSPACE_DIR" >/dev/null
$BAZEL_BIN test "$TARGET_LABEL" "$@"
