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
