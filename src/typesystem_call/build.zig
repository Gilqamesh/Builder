const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const call_dep = b.dependency("call", .{ .target = target, .optimize = optimize });
    const typesystem_dep = b.dependency("typesystem", .{ .target = target, .optimize = optimize });

    const lib = b.addStaticLibrary(.{
        .name = "typesystem_call",
        .target = target,
        .optimize = optimize,
    });
    lib.addCSourceFiles(.{ .files = &.{ "typesystem_call.cpp" }, .flags = &.{ "-std=c++23" } });
    lib.addIncludePath(b.path("."));
    lib.addIncludePath(call_dep.path(""));
    lib.addIncludePath(typesystem_dep.path(""));
    lib.linkLibrary(call_dep.artifact("call"));
    lib.linkLibrary(typesystem_dep.artifact("typesystem"));
    lib.linkLibCpp();
    b.installArtifact(lib);
}
