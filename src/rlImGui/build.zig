const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const imgui_dep = b.dependency("imgui", .{
        .target = target,
        .optimize = optimize,
    });

    const raylib_dep = b.dependency("raylib_wrapper", .{
        .target = target,
        .optimize = optimize,
    });

    const lib = b.addStaticLibrary(.{
        .name = "rlImGui",
        .target = target,
        .optimize = optimize,
    });

    lib.addCSourceFiles(.{
        .files = &.{ "rlImGui.cpp" },
        .flags = &.{ "-std=c++23" },
    });

    lib.addIncludePath(b.path("."));         // local rlImGui/
    lib.addIncludePath(b.path("extras"));    // rlImGui/extras
    lib.addIncludePath(imgui_dep.path(""));  // imgui headers
    lib.addIncludePath(raylib_dep.path("src"));  // raylib’s headers

    lib.linkLibrary(imgui_dep.artifact("imgui"));
    lib.linkLibrary(raylib_dep.artifact("raylib"));

    lib.linkLibCpp();
    b.installArtifact(lib);
}
