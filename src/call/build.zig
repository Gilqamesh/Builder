const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const lib = b.addStaticLibrary(.{
        .name = "call",
        .target = target,
        .optimize = optimize,
    });
    lib.addCSourceFiles(.{ .files = &.{ b.path("call.cpp") }, .flags = &.{ "-std=c++23" } });
    lib.addIncludePath(b.path("."));
    lib.linkLibCpp();
    b.installArtifact(lib);
}
