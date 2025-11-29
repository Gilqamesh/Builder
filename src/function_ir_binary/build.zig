const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const function_ir_dep = b.dependency("function_ir", .{ .target = target, .optimize = optimize });

    const lib = b.addStaticLibrary(.{
        .name = "function_ir_binary",
        .target = target,
        .optimize = optimize,
    });
    lib.addCSourceFiles(.{ .files = &.{ "function_ir_binary.cpp" }, .flags = &.{} });
    lib.addIncludePath(b.path("."));
    lib.addIncludePath(function_ir_dep.path(""));
    lib.linkLibrary(function_ir_dep.artifact("function_ir"));
    lib.linkLibCpp();
    b.installArtifact(lib);
}
