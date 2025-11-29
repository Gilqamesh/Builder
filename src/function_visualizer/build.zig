const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const rlimgui_dep = b.dependency("rlImGui", .{});
    const imgui_dep = b.dependency("imgui", .{});
    const function_alu_dep = b.dependency("function_alu", .{});
    const function_compound_dep = b.dependency("function_compound", .{});
    const function_ir_file_repository_dep = b.dependency("function_ir_file_repository", .{});
    const function_repository_dep = b.dependency("function_repository", .{});
    const function_visualizer_editor_dep = b.dependency("function_visualizer_editor", .{});

    const exe = b.addExecutable(.{
        .name = "function_visualizer",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
        }),
    });

    exe.addCSourceFiles(.{
        .files = &.{ "function_visualizer.cpp" },
        .flags = &.{ "-std=c++23" },
    });

    exe.addIncludePath(b.path("."));
    exe.addIncludePath(rlimgui_dep.path(""));
    exe.addIncludePath(imgui_dep.path(""));
    exe.addIncludePath(function_alu_dep.path(""));
    exe.addIncludePath(function_compound_dep.path(""));
    exe.addIncludePath(function_ir_file_repository_dep.path(""));
    exe.addIncludePath(function_repository_dep.path(""));
    exe.addIncludePath(function_visualizer_editor_dep.path(""));

    exe.linkLibrary(rlimgui_dep.artifact("rlImGui"));
    exe.linkLibrary(imgui_dep.artifact("imgui"));
    exe.linkLibrary(function_alu_dep.artifact("function_alu"));
    exe.linkLibrary(function_compound_dep.artifact("function_compound"));
    exe.linkLibrary(function_ir_file_repository_dep.artifact("function_ir_file_repository"));
    exe.linkLibrary(function_repository_dep.artifact("function_repository"));
    exe.linkLibrary(function_visualizer_editor_dep.artifact("function_visualizer_editor"));

    exe.linkSystemLibrary("raylib");
    exe.linkSystemLibrary("glfw");
    exe.linkSystemLibrary("GL");
    exe.linkSystemLibrary("dl");
    exe.linkSystemLibrary("pthread");
    exe.linkLibCpp();

    b.installArtifact(exe);

    const run_cmd = b.addRunArtifact(exe);
    const run_step = b.step("run", "Run function_visualizer");
    run_step.dependOn(&run_cmd.step);
}
