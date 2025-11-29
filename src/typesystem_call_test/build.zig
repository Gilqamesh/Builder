const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const dep = b.dependency("typesystem_call", .{ .target = target, .optimize = optimize });

    const exe = b.addExecutable(.{
        .name = "typesystem_call_test",
        .target = target,
        .optimize = optimize,
    });
    exe.addCSourceFiles(.{ .files = &.{ "typesystem_call_test.cpp" }, .flags = &.{} });
    exe.addIncludePath(.{ .path = "." });
    exe.addIncludePath(dep.path("."));
    exe.linkLibrary(dep.artifact("typesystem_call"));
    exe.linkSystemLibrary("gtest");
    exe.linkSystemLibrary("gtest_main");
    exe.linkLibCpp();
    b.installArtifact(exe);

    const run_cmd = b.addRunArtifact(exe);
    const test_step = b.step("test", "Run typesystem_call tests");
    test_step.dependOn(&run_cmd.step);
}
