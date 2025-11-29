const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const function_ir_dep = b.dependency("function_ir", .{ .target = target, .optimize = optimize });
    const typesystem_dep = b.dependency("typesystem", .{ .target = target, .optimize = optimize });

    const lib = b.addStaticLibrary(.{
        .name = "function",
        .target = target,
        .optimize = optimize,
    });
    lib.addCSourceFiles(.{ .files = &.{ b.path("function.cpp") }, .flags = &.{ "-std=c++23" } });
    lib.addIncludePath(b.path("."));
    lib.addIncludePath(function_ir_dep.path(""));
    lib.addIncludePath(typesystem_dep.path(""));
    lib.linkLibrary(function_ir_dep.artifact("function_ir"));
    lib.linkLibrary(typesystem_dep.artifact("typesystem"));
    lib.linkLibCpp();
    b.installArtifact(lib);
}
