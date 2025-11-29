const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const function_dep = b.dependency("function", .{ .target = target, .optimize = optimize });

    const exe = b.addExecutable(.{
        .name = "function_test",
        .target = target,
        .optimize = optimize,
    });
    exe.addCSourceFiles(.{ .files = &.{ "function_test.cpp" }, .flags = &.{} });
    exe.addIncludePath(b.path("."));
    exe.addIncludePath(function_dep.path(""));
    exe.linkLibrary(function_dep.artifact("function"));
    exe.linkSystemLibrary("gtest");
    exe.linkSystemLibrary("gtest_main");
    exe.linkLibCpp();
    b.installArtifact(exe);

    const run_cmd = b.addRunArtifact(exe);
    const test_step = b.step("test", "Run function tests");
    test_step.dependOn(&run_cmd.step);
}
