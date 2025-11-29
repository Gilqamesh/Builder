const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const compound_dep = b.dependency("function_compound", .{ .target = target, .optimize = optimize });

    const exe = b.addExecutable(.{
        .name = "function_compound_test",
        .target = target,
        .optimize = optimize,
    });
    exe.addCSourceFiles(.{ .files = &.{ "function_compound_test.cpp" }, .flags = &.{} });
    exe.addIncludePath(.{ .path = "." });
    exe.addIncludePath(compound_dep.path("."));
    exe.linkLibrary(compound_dep.artifact("function_compound"));
    exe.linkSystemLibrary("gtest");
    exe.linkSystemLibrary("gtest_main");
    exe.linkLibCpp();
    b.installArtifact(exe);

    const run_cmd = b.addRunArtifact(exe);
    const test_step = b.step("test", "Run function_compound tests");
    test_step.dependOn(&run_cmd.step);
}
