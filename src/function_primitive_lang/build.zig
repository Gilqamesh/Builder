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
    lib.addCSourceFiles(.{ .files = &.{ "function_primitive_lang.cpp" }, .flags = &.{} });
    lib.addIncludePath(b.path("."));
    lib.addIncludePath(function_dep.path(""));
    lib.linkLibrary(function_dep.artifact("function"));
    lib.linkLibCpp();
    b.installArtifact(lib);
}
