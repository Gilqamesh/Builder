const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const function_dep = b.dependency("function", .{ .target = target, .optimize = optimize });
    const function_id_dep = b.dependency("function_id", .{ .target = target, .optimize = optimize });

    const lib = b.addStaticLibrary(.{
        .name = "function_call_repository",
        .target = target,
        .optimize = optimize,
    });
    lib.addCxxSourceFile(.{ .file = "function_call_repository.cpp", .flags = &.{} });
    lib.addIncludePath(.{ .path = "." });
    lib.addIncludePath(function_dep.path("."));
    lib.addIncludePath(function_id_dep.path("."));
    lib.linkLibrary(function_dep.artifact("function"));
    lib.linkLibrary(function_id_dep.artifact("function_id"));
    lib.linkLibCpp();
    b.installArtifact(lib);
}
