const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const function_dep = b.dependency("function", .{});
    const typesystem_dep = b.dependency("typesystem", .{});

    const lib = b.addLibrary(.{
        .name = "function_alu",
        .linkage = .static,
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
        }),
    });

    lib.addCSourceFiles(.{
        .files = &.{ "function_alu.cpp" },
        .flags = &.{ "-std=c++23" },
    });

    lib.addIncludePath(b.path("."));
    lib.addIncludePath(function_dep.path(""));
    lib.addIncludePath(typesystem_dep.path(""));

    lib.linkLibrary(function_dep.artifact("function"));
    lib.linkLibrary(typesystem_dep.artifact("typesystem"));
    lib.linkLibCpp();

    b.installArtifact(lib);
}
