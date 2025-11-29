const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const lib = b.addStaticLibrary(.{
        .name = "call",
        .target = target,
        .optimize = optimize,
    });
    lib.addCSourceFiles(.{ .files = &.{ "call.cpp" }, .flags = &.{} });
    lib.addIncludePath(.{ .path = "." });
    lib.linkLibCpp();
    b.installArtifact(lib);
}
