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

MODULES=()
while IFS= read -r -d '' path; do
    MODULES+=("$(basename "$path")")
done < <(find "$REPO_ROOT/src" -maxdepth 1 -mindepth 1 -type d -print0 | sort -z)

cat > "$WORKSPACE_DIR/MODULE.bazel" <<'EOF_MODULE'
module(name = "builder_local", version = "0.0.0")

bazel_dep(name = "googletest", version = "1.14.0", repo_name = "com_google_googletest")
bazel_dep(name = "raylib", version = "5.0.0")
EOF_MODULE

for module in "${MODULES[@]}"; do
    if [[ -f "$REPO_ROOT/src/$module/MODULE.bazel" ]]; then
        cat >> "$WORKSPACE_DIR/MODULE.bazel" <<EOF_MODULE_ENTRY
bazel_dep(name = "$module", version = "0.0.0")
local_path_override(name = "$module", path = "$REPO_ROOT/src/$module")
EOF_MODULE_ENTRY
    fi
done

touch "$WORKSPACE_DIR/WORKSPACE.bazel"

target_input=$1
shift || true

if [[ "$target_input" == @* ]]; then
    TARGET_LABEL="$target_input"
else
    trimmed=${target_input#//}
    module_name=${trimmed%%:*}
    target_name=${trimmed#*:}
    if [[ "$module_name" == "$trimmed" ]]; then
        echo "Targets must be of the form //module:target or @module//:target" >&2
        exit 1
    fi
    TARGET_LABEL="@${module_name}//:${target_name}"
fi

pushd "$WORKSPACE_DIR" >/dev/null
$BAZEL_BIN --enable_bzlmod test "$TARGET_LABEL" "$@"
popd >/dev/null
