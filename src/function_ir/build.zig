const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const function_id_dep = b.dependency("function_id", .{ .target = target, .optimize = optimize });

    const lib = b.addStaticLibrary(.{
        .name = "function_ir",
        .target = target,
        .optimize = optimize,
    });
    lib.addCSourceFiles(.{ .files = &.{ b.path("function_ir.cpp") }, .flags = &.{ "-std=c++23" } });
    lib.addIncludePath(b.path("."));
    lib.addIncludePath(function_id_dep.path(""));
    lib.linkLibrary(function_id_dep.artifact("function_id"));
    lib.linkLibCpp();
    b.installArtifact(lib);
}
