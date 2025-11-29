const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const imgui_dep = b.dependency("imgui", .{});
    const rlimgui_dep = b.dependency("rlImGui", .{});

    const lib = b.addLibrary(.{
        .name = "function_visualizer_editor",
        .linkage = .static,
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
        }),
    });

    lib.addCSourceFiles(.{
        .files = &.{ "function_visualizer_editor.cpp" },
        .flags = &.{ "-std=c++23" },
    });

    lib.addIncludePath(b.path("."));
    lib.addIncludePath(imgui_dep.path(""));
    lib.addIncludePath(rlimgui_dep.path(""));

    lib.linkLibrary(imgui_dep.artifact("imgui"));
    lib.linkLibrary(rlimgui_dep.artifact("rlImGui"));

    lib.linkSystemLibrary("raylib");
    lib.linkSystemLibrary("glfw");
    lib.linkSystemLibrary("GL");
    lib.linkSystemLibrary("dl");
    lib.linkSystemLibrary("pthread");
    lib.linkLibCpp();

    b.installArtifact(lib);
}
