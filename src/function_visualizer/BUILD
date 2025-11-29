package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "function_visualizer",
    srcs = ["function_visualizer.cpp"],
    deps = [
        "//src/rlImGui:rlImGui",
        "//src/imgui:imgui",
        "//src/function_alu:function_alu",
        "//src/function_compound:function_compound",
        "//src/function_ir_file_repository:function_ir_file_repository",
        "//src/function_repository:function_repository",
        "//src/function_visualizer_editor:function_visualizer_editor",
    ],
)
