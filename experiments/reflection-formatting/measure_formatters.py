#!/usr/bin/env python3
"""Measure the same formatter workload against published baseline or migrated headers."""
import argparse
import json
import os
from pathlib import Path
import statistics
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument("phase", choices=["baseline", "reflected"])
args = parser.parse_args()
artifacts = Path(os.environ["BUILDER_ARTIFACT_ROOT"])
output = artifacts / "validation" / (args.phase + "-formatters")
output.mkdir()
interfaces = sorted(path.resolve() for path in artifacts.glob("m*/latest/interface/install"))
(output / "interfaces.json").write_text(json.dumps([str(path) for path in interfaces], indent=2) + "\n")
source = output / "measure.cpp"
source.write_text(r'''#include <m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer/geometry.h>
#include <m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer/framebuffer.h>
#include <m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer/material.h>
#include <m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer/profiling_metrics.h>
#include <m03gkcdy62bnz808pmk4uzkjra_glfw/monitor.h>
#include <m03gkcdy62bnz808pmk4uzkjra_glfw/window_creation_settings.h>
#include <m03gkcdy62bnz808pmk4uzkjra_glfw/input.h>
#include <chrono>
#include <cstddef>
#include <format>
#include <print>
namespace renderer = m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer;
namespace glfw = m03gkcdy62bnz808pmk4uzkjra_glfw;
int main() {
    constexpr std::size_t iterations = 1000000;
    std::size_t bytes = 0;
    const auto start = std::chrono::steady_clock::now();
    for (std::size_t index = 0; index < iterations; ++index) {
        bytes += std::format("{} {} {}", renderer::index_range_t{index, 12},
            renderer::rgba8_t{1, 2, 3, 255}, glfw::video_mode_t{128, 256, 8, 8, 8, 60}).size();
    }
    const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start).count();
    std::println("{} {} {}", elapsed, bytes, iterations);
    std::println("{}", renderer::index_range_t{4, 12});
    std::println("{}", renderer::rgba8_t{1, 2, 3, 255});
    std::println("{}", renderer::stencil_state_t{});
    std::println("{}", renderer::blend_equation_t{});
    std::println("{}", glfw::video_mode_t{128, 256, 8, 8, 8, 60});
    std::println("{}", renderer::comparison_t::less_equal);
    std::println("{}", static_cast<renderer::comparison_t>(42));
    std::println("{}", static_cast<renderer::color_mask_t>(5));
    std::println("{}", renderer::raster_metrics_t{});
    std::println("{}", glfw::client_api_t::opengl);
    std::println("{}", glfw::gamepad_button_t::button_cross);
}
''')
compiler = os.environ.get("CXX", "g++")
binary = output / "measure"
compile_samples = []
for index in range(3):
    timing = output / ("compile-" + str(index) + ".txt")
    command = ["/usr/bin/time", "-o", str(timing), "-f", "%e %M", compiler,
        "-std=c++26", "-freflection", "-O2", *["-I" + str(path) for path in interfaces],
        str(source), "-o", str(binary)]
    (output / "compile-command.json").write_text(json.dumps(command, indent=2) + "\n")
    subprocess.run(command, check=True)
    wall, memory = timing.read_text().split()
    compile_samples.append({"wall_seconds": float(wall), "peak_rss_kb": int(memory)})
runtime_samples = []
for index in range(5):
    result = subprocess.check_output([str(binary)], text=True)
    (output / ("run-" + str(index) + ".txt")).write_text(result)
    elapsed, size, iterations = map(int, result.splitlines()[0].split())
    runtime_samples.append({"ns_per_iteration": elapsed / iterations, "bytes": size})
    (output / "output.txt").write_text("\n".join(result.splitlines()[1:]) + "\n")
report = {"compiler": subprocess.check_output([compiler, "--version"], text=True).splitlines()[0],
    "compile_samples": compile_samples, "runtime_samples": runtime_samples,
    "median_compile_seconds": statistics.median(row["wall_seconds"] for row in compile_samples),
    "median_peak_rss_kb": statistics.median(row["peak_rss_kb"] for row in compile_samples),
    "median_ns_per_iteration": statistics.median(row["ns_per_iteration"] for row in runtime_samples),
    "binary_bytes": binary.stat().st_size}
(output / "results.json").write_text(json.dumps(report, indent=2) + "\n")
print(json.dumps(report, indent=2))
