load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "gtest",
    urls = ["https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz"],
    strip_prefix = "googletest-1.15.2",
    sha256 = "",
)

http_archive(
    name = "raylib",
    urls = ["https://github.com/raysan5/raylib/archive/refs/tags/5.0.tar.gz"],
    strip_prefix = "raylib-5.0",
    sha256 = "",
    build_file_content = """
cc_library(
    name = "raylib",
    srcs = glob(["src/*.c"]),
    hdrs = glob(["src/*.h"]),
    includes = ["src"],
    copts = ["-D_GNU_SOURCE"],
    linkopts = ["-lm", "-lpthread", "-ldl", "-lrt", "-lX11", "-lGL"],
    visibility = ["//visibility:public"],
)
""",
)
