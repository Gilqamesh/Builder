package(default_visibility = ["//visibility:public"])

cc_library(
    name = "rlImGui",
    srcs = ["rlImGui.cpp"],
    hdrs = glob(["*.h", "extras/*.h"]),
    includes = [".", "extras"],
    deps = [
        "//src/imgui:imgui",
        "@raylib//:raylib",
    ],
)
