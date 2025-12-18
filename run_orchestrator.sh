#!/usr/bin/env bash
set -euo pipefail

target_module=${1}
latest_builder=$(printf "%s\n" ./artifacts/builder/builder@* | sort | tail -n1)

exec_command=(
  "$latest_builder/orchestrator"
  .
  "modules"
  ${target_module}
  artifacts
)
echo ${exec_command[@]}
exec ${exec_command[@]}

