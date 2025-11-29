load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def _raylib_ext_impl(module_ctx):
    http_archive(
        name = "raylib",
        urls = [
            "https://github.com/raysan5/raylib/archive/refs/tags/5.0.tar.gz",
        ],
        strip_prefix = "raylib-5.0",
        build_file = module_ctx.path("raylib.BUILD"),
    )


raylib_ext = module_extension(implementation = _raylib_ext_impl)
