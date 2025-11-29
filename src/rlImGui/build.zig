const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const imgui_dep = b.dependency("imgui", .{ .target = target, .optimize = optimize });

    const lib = b.addStaticLibrary(.{
        .name = "rlImGui",
        .target = target,
        .optimize = optimize,
    });
    lib.addCSourceFiles(.{ .files = &.{ "rlImGui.cpp" }, .flags = &.{} });
    lib.addIncludePath(.{ .path = "." });
    lib.addIncludePath(.{ .path = "extras" });
    lib.addIncludePath(imgui_dep.path("."));
    lib.linkLibrary(imgui_dep.artifact("imgui"));
    lib.linkSystemLibrary("raylib");
    lib.linkSystemLibrary("glfw");
    lib.linkSystemLibrary("GL");
    lib.linkSystemLibrary("dl");
    lib.linkSystemLibrary("pthread");
    lib.linkLibCpp();
    b.installArtifact(lib);
}
