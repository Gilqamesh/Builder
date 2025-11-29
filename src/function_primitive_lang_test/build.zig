const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const dep = b.dependency("function_primitive_lang", .{ .target = target, .optimize = optimize });

    const exe = b.addExecutable(.{
        .name = "function_primitive_lang_test",
        .target = target,
        .optimize = optimize,
    });
    exe.addCSourceFiles(.{ .files = &.{ "function_primitive_lang_test.cpp" }, .flags = &.{ "-std=c++23" } });
    exe.addIncludePath(b.path("."));
    exe.addIncludePath(dep.path(""));
    exe.linkLibrary(dep.artifact("function_primitive_lang"));
    exe.linkSystemLibrary("gtest");
    exe.linkSystemLibrary("gtest_main");
    exe.linkLibCpp();
    b.installArtifact(exe);

    const run_cmd = b.addRunArtifact(exe);
    const test_step = b.step("test", "Run function_primitive_lang tests");
    test_step.dependOn(&run_cmd.step);
}
