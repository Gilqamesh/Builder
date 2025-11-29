#!/usr/bin/env bash
set -euo pipefail

if [ $# -lt 1 ]; then
  echo "Usage: $0 <crate>";
  exit 1
fi

crate="$1"
shift || true

cargo run -p "$crate" -- "$@"
