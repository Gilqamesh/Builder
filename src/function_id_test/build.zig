const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const function_id_dep = b.dependency("function_id", .{ .target = target, .optimize = optimize });

    const exe = b.addExecutable(.{
        .name = "function_id_test",
        .target = target,
        .optimize = optimize,
    });
    exe.addCSourceFiles(.{ .files = &.{ "function_id_test.cpp" }, .flags = &.{} });
    exe.addIncludePath(b.path("."));
    exe.addIncludePath(function_id_dep.path(""));
    exe.linkLibrary(function_id_dep.artifact("function_id"));
    exe.linkSystemLibrary("gtest");
    exe.linkSystemLibrary("gtest_main");
    exe.linkLibCpp();
    b.installArtifact(exe);

    const run_cmd = b.addRunArtifact(exe);
    const test_step = b.step("test", "Run function_id tests");
    test_step.dependOn(&run_cmd.step);
}
