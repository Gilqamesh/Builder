#!/usr/bin/env python3
"""Compose the isolated layout from the two experimental checkouts."""
import json
import os
from pathlib import Path
import subprocess

projects = Path(__file__).resolve().parents[3]
repositories = [projects / "Builder-reflection-formatting", projects / "Builder-Modules-reflection-formatting"]
layout = projects / "Builder-Layout-reflection-formatting"
artifacts = projects / "Builder-Artifacts-reflection-formatting"
branch = "experiment/reflection-formatting"
links = {}
sources = {}
for repository in repositories:
    current = subprocess.check_output(["git", "-C", str(repository), "branch", "--show-current"], text=True).strip()
    if current != branch:
        raise SystemExit(str(repository) + " must be on " + branch)
    sources[repository.name] = subprocess.check_output(["git", "-C", str(repository), "rev-parse", "HEAD"], text=True).strip()
    for workspace in ["ws0", "ws1", "ws2"]:
        source = repository / workspace
        if not source.is_dir():
            continue
        for module in sorted(source.iterdir()):
            if not module.is_dir():
                continue
            destination = layout / workspace / module.name
            if destination in links:
                raise SystemExit("Duplicate module: " + module.name)
            links[destination] = Path("../..") / repository.name / workspace / module.name
for name in ["AGENTS.md", ".github", "docs"]:
    links[layout / name] = Path("..") / repositories[0].name / name
links[layout / "artifacts"] = Path("..") / artifacts.name
links[layout / "run-toolchain"] = Path("../Builder-reflection-formatting/experiments/reflection-formatting/run-toolchain")
for destination, target in links.items():
    if os.path.lexists(destination) and not (destination.is_symlink() and destination.readlink() == target):
        raise SystemExit("Refusing to replace " + str(destination))
runner = '#!/usr/bin/env bash\nset -euo pipefail\nreadonly workspace="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"\nexec "$workspace/run-toolchain" "$workspace/cli" "$@"\n'
if (layout / "run-builder").exists() and (layout / "run-builder").read_text() != runner:
    raise SystemExit("Refusing to replace run-builder")
artifacts.mkdir(exist_ok=True)
for workspace in ["ws0", "ws1", "ws2"]:
    (layout / workspace).mkdir(parents=True, exist_ok=True)
for destination, target in links.items():
    if not os.path.lexists(destination):
        destination.symlink_to(target)
if not (layout / "run-builder").exists():
    (layout / "run-builder").write_text(runner)
(layout / "run-builder").chmod(0o755)
manifest = artifacts / "experiment.json"
if not manifest.exists():
    manifest.write_text(json.dumps({"branch": branch, "sources": sources}, indent=2) + "\n")
(artifacts / "validation").mkdir(exist_ok=True)
print("Verified", len(links), "layout links under", layout)
