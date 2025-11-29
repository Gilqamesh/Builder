const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const function_alu_dep = b.dependency("function_alu", .{ .target = target, .optimize = optimize });

    const exe = b.addExecutable(.{
        .name = "function_alu_test",
        .target = target,
        .optimize = optimize,
    });
    exe.addCSourceFiles(.{ .files = &.{ "function_alu_test.cpp" }, .flags = &.{ "-std=c++23" } });
    exe.addIncludePath(b.path("."));
    exe.addIncludePath(function_alu_dep.path(""));
    exe.linkLibrary(function_alu_dep.artifact("function_alu"));
    exe.linkSystemLibrary("gtest");
    exe.linkSystemLibrary("gtest_main");
    exe.linkLibCpp();
    b.installArtifact(exe);

    const run_cmd = b.addRunArtifact(exe);
    const test_step = b.step("test", "Run function_alu tests");
    test_step.dependOn(&run_cmd.step);
}
