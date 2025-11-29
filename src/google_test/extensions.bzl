load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def _gtest_impl(module_ctx):
    http_archive(
        name = "com_google_googletest",
        urls = [
            "https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip",
        ],
        strip_prefix = "googletest-1.15.2",
    )


gtest = module_extension(implementation = _gtest_impl)
