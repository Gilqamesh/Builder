const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const lib = b.addStaticLibrary(.{
        .name = "function_id",
        .target = target,
        .optimize = optimize,
    });
    lib.addCSourceFiles(.{ .files = &.{ "function_id.cpp" }, .flags = &.{} });
    lib.addIncludePath(b.path("."));
    lib.linkLibCpp();
    b.installArtifact(lib);
}
