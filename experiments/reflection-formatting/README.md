# Reflection formatting experiment

Both independent checkouts use `experiment/reflection-formatting`. The layout and
artifacts are isolated from the normal Builder workspace:

```text
/home/gilqamesh/Projects/
├── Builder-reflection-formatting/          # Builder Git checkout
├── Builder-Modules-reflection-formatting/  # Builder-Modules Git checkout
├── Builder-Layout-reflection-formatting/   # ws0/ws1/ws2 module symlinks and launchers
└── Builder-Artifacts-reflection-formatting/# GCC 16 builds, measurements and logs
```

`configure.py` composes and verifies the layout from those two checkouts. It checks
both branches and rejects conflicting links. It does not switch branches or alter
the normal checkouts. The two repositories have independent commit histories; use
the paired revisions recorded in the artifact directory's `experiment.json`.

## Toolchain and running

The Containerfile pins the GCC base image by digest. The validated image contains
GCC 16.2.0, matching libstdc++, Boost, and the X11/Wayland/OpenGL dependencies needed
by the pilots. Container-local `/usr/lib64` symlinks satisfy the CLI shell's
existing readline/history lookup. The local image ID is recorded in `toolchain-image`; package versions
are captured in `validation/toolchain-packages.txt`. Apt packages are not pinned to
a Debian snapshot, so rebuilding the container can change those package versions.

To prepare the layout and build the image from these checkouts:

```bash
cd /home/gilqamesh/Projects/Builder-reflection-formatting
python3 experiments/reflection-formatting/configure.py
podman build --tag localhost/builder-reflection-formatting:gcc16 \
  --iidfile ../Builder-Artifacts-reflection-formatting/toolchain-image \
  --file experiments/reflection-formatting/Containerfile \
  experiments/reflection-formatting
cd ../Builder-Layout-reflection-formatting
./run-toolchain make \
  -f ws0/m03gagbhst621faiop1rztfkqp_builder_cli/bootstrap.mk \
  WORKSPACE_ROOT_DIR="$PWD" bootstrap
```

All experimental C++ compilation uses `-std=c++26 -freflection`. Run the workspace
through its container launcher so the compiler, runtime libraries, workspace root,
and artifact root agree:

```bash
./run-builder m03gtrxnmqqa2t7zxpijo222n6_formatting
./run-toolchain xvfb-run -a ./cli m03gkcdy62bnz808pmk4uzkjra_glfw:test
```

The launcher caps each container at two CPUs and 8 GiB and mounts the four
experimental directories. It leaves the host compiler and normal build artifacts
unchanged. `--init` is required for Xvfb's startup signalling in the container.

## Shared API

The public contract is in
[`m03gtrxnmqqa2t7zxpijo222n6_formatting/api.h`](../../ws1/m03gtrxnmqqa2t7zxpijo222n6_formatting/api.h).
A type owner selects structural formatting with its explicit specialization:

```cpp
#include <m03gtrxnmqqa2t7zxpijo222n6_formatting/api.h>

struct extent_t {
    std::size_t width;
    std::size_t height;
};

template <>
struct std::formatter<extent_t>
    : m03gtrxnmqqa2t7zxpijo222n6_formatting::reflected_formatter_t {};

// std::format("{}", extent_t{128, 256})
// extent_t{width=128, height=256}
```

Records use public direct bases and members in declaration order. The complete
record format string is generated at compile time and passed to one `format_to`
call; no member list is maintained by the type owner. Base subobjects use C++26
splicing. Nested values use their own formatters. Enums print a declared name or
`invalid(n)`; aliases choose the first matching declaration. Only empty format
specifications and `char` output are supported.

Private or protected subobjects, unions, and unformattable subobjects produce a
compile-time diagnostic when structural formatting is used. Provide a custom
formatter for those types. This deliberately avoids silently incomplete resource
summaries and makes no attempt to traverse pointer ownership graphs. Structural
output is diagnostic text, not a stable serialization format.

A custom specialization can inherit the shared parser and define its own `format`
method. Mathematical notation, resource summaries, semantic enum labels, flags,
and derived profiler values stay with their type owners. Foundation modules in
`ws0` retain their current formatters because they cannot depend on `ws1`.

## Pilots

The pilots had 45 and 27 formatter definitions respectively before migration,
including the GLFW CLI formatter. They exercise renderer state, enum fallbacks,
resource summaries, input aliases, and real window/context creation.

| Module | Structural migrations | Custom presentation retained |
|---|---|---|
| `m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer` | `index_range_t`, `rgba8_t`, `stencil_state_t`, `blend_equation_t`, and eight enums | Resource summaries, mathematical values, flags, special enum behavior and all profiler metrics |
| `m03gkcdy62bnz808pmk4uzkjra_glfw` | `video_mode_t` | Remaining 26 formatter bodies retain their output and share the empty-specification parser |

Five record representations now include the type name, use `name=value`, and use
source identifiers such as `red_bits`. The eight migrated renderer enums preserve
their existing names and numeric fallback. Custom GLFW exception behavior is
preserved; malformed format specifications now receive the shared diagnostic.

## Validation and measurements

Builder runs each module's `test/public_api.cpp` during library installation. The
new shared module tests records, enums, nested custom formatting, bases (including
a virtual diamond), bit-fields, noncopyable and borrowed members, template types,
output iterators, truncation, and invalid specifications.

```bash
# From Builder-Layout-reflection-formatting:
./run-toolchain python3 \
  ws1/m03gtrxnmqqa2t7zxpijo222n6_formatting/test/check_compilation.py
./run-toolchain ./cli m03gl8a1hl8xe3ynm8s2wwfy4u_software_renderer:benchmark \
  --output "$PWD/artifacts/validation/manual-renderer" \
  --size 32 --warmup 1 --samples 3 --runs 2 --report
./run-toolchain xvfb-run -a ./cli m03gkcdy62bnz808pmk4uzkjra_glfw:test
./run-toolchain xvfb-run -a ./cli m03gkcdy62bnz808pmk4uzkjra_glfw \
  --script "$PWD/ws2/m03gkcdy62bnz808pmk4uzkjra_glfw/test/settings_test.txt"
```

`build_targets.py` uses the existing public phase API to install binary targets
without launching interactive applications. Library validation still runs:

```bash
./run-toolchain python3 \
  ../Builder-reflection-formatting/experiments/reflection-formatting/build_targets.py \
  m03gl22hn0dqmosreqjie9tg5m_opengl_renderer \
  m03gl8ffb2e842ezg4fboslgqu_glfw_window_renderer \
  m03gilsfsv3k34ej14ytz8a29k_tower_defense_game \
  m03gm4hnyxwh4vcy1l7xonx52c_module_editor
```

`measure_formatters.py baseline|reflected` compiles and runs one fixed formatter
workload against the interfaces currently published in the experimental artifacts.
The label does not switch repository revisions. Capture the baseline before
migration and the reflected measurement after building migrated modules. Each run
requires an unused output directory, pins resolved interface paths in its evidence,
and records three compile samples, five runtime samples, output text and binary
size. Avoid concurrent builds when collecting timing data.

The validation report and raw evidence live in
`Builder-Artifacts-reflection-formatting/validation/REPORT.md`. Small container
benchmarks are indicative; the changed diagnostic output length also affects the
formatting workload. Real desktop presentation, physical input devices, and other
compilers require separate validation.

## Observed result (2026-09-07)

The completed pilot replaces 13 formatter bodies and shares parsing in 26 custom
formatters, with 359 fewer lines across the pilot changes including new tests.
Both pilots, all four direct application dependents, and Builder CLI validation
pass with GCC 16.2.0. All 13 renderer benchmark workloads preserve profiler counter
output. GLFW runtime, settings, context and CLI smoke checks pass under Xvfb.

| Fixed three-record formatting workload | Handwritten baseline | Reflected |
|---|---:|---:|
| Median compile time (three samples) | 3.35 s | 3.73 s |
| Median compiler peak RSS | 391,492 KiB | 399,616 KiB |
| Median runtime per iteration (five samples) | 558.58 ns | 489.42 ns |
| Executable size | 162,776 bytes | 166,056 bytes |

Each runtime sample formats one million combinations of `index_range_t`, `rgba8_t`
and `video_mode_t`. The reflected output is about 8.8% longer. These observations
support further experimentation, not a general performance guarantee. Full-build
wall times are not compared because baseline and migration runs had different
cache states. Hardware input and visible desktop presentation remain unverified.
