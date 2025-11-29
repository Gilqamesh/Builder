const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const call_dep = b.dependency("call", .{ .target = target, .optimize = optimize });

    const exe = b.addExecutable(.{
        .name = "call_test",
        .target = target,
        .optimize = optimize,
    });
    exe.addCSourceFiles(.{ .files = &.{ "call_test.cpp" }, .flags = &.{} });
    exe.addIncludePath(b.path("."));
    exe.addIncludePath(call_dep.path(""));
    exe.linkLibrary(call_dep.artifact("call"));
    exe.linkSystemLibrary("gtest");
    exe.linkSystemLibrary("gtest_main");
    exe.linkLibCpp();
    b.installArtifact(exe);

    const run_cmd = b.addRunArtifact(exe);
    const test_step = b.step("test", "Run call tests");
    test_step.dependOn(&run_cmd.step);
}
