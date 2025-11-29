const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const dep = b.dependency("function_primitive_cpp", .{ .target = target, .optimize = optimize });

    const exe = b.addExecutable(.{
        .name = "function_primitive_cpp_test",
        .target = target,
        .optimize = optimize,
    });
    exe.addCSourceFiles(.{ .files = &.{ "function_primitive_cpp_test.cpp" }, .flags = &.{} });
    exe.addIncludePath(".");
    exe.addIncludePath(dep.path(""));
    exe.linkLibrary(dep.artifact("function_primitive_cpp"));
    exe.linkSystemLibrary("gtest");
    exe.linkSystemLibrary("gtest_main");
    exe.linkLibCpp();
    b.installArtifact(exe);

    const run_cmd = b.addRunArtifact(exe);
    const test_step = b.step("test", "Run function_primitive_cpp tests");
    test_step.dependOn(&run_cmd.step);
}
