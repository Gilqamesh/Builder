load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

RAYLIB_BUILD_FILE = """
cc_library(
    name = "raylib",
    srcs = glob(["src/*.c"]),
    hdrs = glob(["src/*.h"]),
    includes = ["src"],
    copts = [
        "-DPLATFORM_DESKTOP",
        "-fPIC",
    ],
    linkopts = [
        "-lm",
        "-lpthread",
        "-ldl",
        "-lrt",
        "-lGL",
        "-lX11",
        "-lXrandr",
        "-lXinerama",
        "-lXcursor",
        "-lXi",
    ],
    visibility = ["//visibility:public"],
)
"""

def _raylib_ext_impl(module_ctx):
    http_archive(
        name = "raylib",
        urls = ["https://github.com/raysan5/raylib/archive/refs/tags/5.0.tar.gz"],
        strip_prefix = "raylib-5.0",
        build_file_content = RAYLIB_BUILD_FILE,
    )

raylib_ext = module_extension(implementation = _raylib_ext_impl)
