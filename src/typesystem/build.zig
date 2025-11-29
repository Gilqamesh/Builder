const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const lib = b.addStaticLibrary(.{
        .name = "typesystem",
        .target = target,
        .optimize = optimize,
    });
    lib.addCSourceFiles(.{ .files = &.{ "typesystem.cpp" }, .flags = &.{} });
    lib.addIncludePath(".");
    lib.linkLibCpp();
    b.installArtifact(lib);
}
