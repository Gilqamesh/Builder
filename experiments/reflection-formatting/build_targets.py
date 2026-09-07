#!/usr/bin/env python3
"""Install experimental binary targets, including library validation, without launching them."""
import argparse
import os
from pathlib import Path
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("targets", nargs="+", help="complete module name, optionally followed by :target")
args = parser.parse_args()
artifacts = Path(os.environ["BUILDER_ARTIFACT_ROOT"])
output = artifacts / "validation" / "build-targets"
output.mkdir(parents=True, exist_ok=True)
source = output / "build.cpp"
source.write_text(r'''#include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>
#include <m03gagbhsujjf63n0w3r2w4q6h_build_phases/build_phases.h>
#include <cstdio>
#include <exception>
#include <print>
#include <string_view>
namespace graph = m03gagbhsp2drqq3gkop8pzfrm_workspace_graph;
namespace phases = m03gagbhsujjf63n0w3r2w4q6h_build_phases;
int main(int argc, char** argv) {
    try {
        const auto context = graph::invocation_context();
        graph::workspace_graph_t workspace(context.workspace_root, context.artifact_root);
        for (int index = 1; index < argc; ++index) {
            const std::string_view argument(argv[index]);
            const auto delimiter = argument.find(':');
            const auto name = argument.substr(0, delimiter);
            const auto target = delimiter == std::string_view::npos ? "cli" : argument.substr(delimiter + 1);
            auto* module = workspace.discover_module(graph::module_name_t(name));
            const auto phase = phases::phase_base_t::make(*module, target);
            const auto installed = phase->install<phases::binary_phase_t>();
            std::println("Installed {}:{} at {}", name, target, installed.target(target).string());
            std::fflush(stdout);
        }
    } catch (const std::exception& error) {
        std::println(stderr, "{}", error.what());
        return 1;
    }
}
''')
seed = artifacts / "m03gagbhst621faiop1rztfkqp_builder_cli/builder/19700101T000000.000000000Z-0000000000000000/install"
binary = output / "build"
subprocess.run([os.environ.get("CXX", "g++"), "-std=c++26", "-freflection",
    "-I" + str(artifacts / "bootstrap/include"), str(source), str(seed / "builder.so"),
    "-Wl,-rpath," + str(seed), "-o", str(binary)], check=True)
subprocess.run([str(binary), *args.targets], check=True)
