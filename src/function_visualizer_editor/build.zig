const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const imgui_dep = b.dependency("imgui", .{ .target = target, .optimize = optimize });
    const rlimgui_dep = b.dependency("rlImGui", .{ .target = target, .optimize = optimize });

    const lib = b.addStaticLibrary(.{
        .name = "function_visualizer_editor",
        .target = target,
        .optimize = optimize,
    });
    lib.addCSourceFiles(.{ .files = &.{ "function_visualizer_editor.cpp" }, .flags = &.{} });
    lib.addIncludePath(.{ .path = "." });
    lib.addIncludePath(imgui_dep.path("."));
    lib.addIncludePath(rlimgui_dep.path("."));
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
