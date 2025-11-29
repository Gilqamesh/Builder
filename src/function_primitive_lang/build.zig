const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const function_dep = b.dependency("function", .{ .target = target, .optimize = optimize });

    const lib = b.addStaticLibrary(.{
        .name = "function_primitive_lang",
        .target = target,
        .optimize = optimize,
    });
    lib.addCSourceFiles(.{ .files = &.{ b.path("function_primitive_lang.cpp") }, .flags = &.{ "-std=c++23" } });
    lib.addIncludePath(b.path("."));
    lib.addIncludePath(function_dep.path(""));
    lib.linkLibrary(function_dep.artifact("function"));
    lib.linkLibCpp();
    b.installArtifact(lib);
}
