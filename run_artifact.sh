#!/usr/bin/env bash
set -euo pipefail

target_module=$1
binary_name=$2

latest_target_module=$(printf "%s\n" artifacts/"$target_module"/"$target_module"@* | sort | tail -n1)
binary="$latest_target_module/$binary_name"

echo ${binary}
exec "$binary"

