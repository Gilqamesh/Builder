const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const repo_dep = b.dependency("function_call_repository", .{ .target = target, .optimize = optimize });

    const exe = b.addExecutable(.{
        .name = "function_call_repository_test",
        .target = target,
        .optimize = optimize,
    });
    exe.addCSourceFiles(.{ .files = &.{ "function_call_repository_test.cpp" }, .flags = &.{ "-std=c++23" } });
    exe.addIncludePath(b.path("."));
    exe.addIncludePath(repo_dep.path(""));
    exe.linkLibrary(repo_dep.artifact("function_call_repository"));
    exe.linkSystemLibrary("gtest");
    exe.linkSystemLibrary("gtest_main");
    exe.linkLibCpp();
    b.installArtifact(exe);

    const run_cmd = b.addRunArtifact(exe);
    const test_step = b.step("test", "Run function_call_repository tests");
    test_step.dependOn(&run_cmd.step);
}
