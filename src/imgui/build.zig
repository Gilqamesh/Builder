const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const lib = b.addStaticLibrary(.{
        .name = "imgui",
        .target = target,
        .optimize = optimize,
    });
    const sources = &[_][]const u8{
        "imgui.cpp",
        "imgui_demo.cpp",
        "imgui_draw.cpp",
        "imgui_tables.cpp",
        "imgui_widgets.cpp",
        "backends/imgui_impl_glfw.cpp",
        "backends/imgui_impl_opengl3.cpp",
    };
    lib.addCSourceFiles(.{ .files = sources, .flags = &.{} });
    lib.addIncludePath(.{ .path = "." });
    lib.addIncludePath(.{ .path = "backends" });
    lib.addIncludePath(.{ .path = "misc" });
    lib.linkSystemLibrary("glfw");
    lib.linkSystemLibrary("GL");
    lib.linkSystemLibrary("dl");
    lib.linkSystemLibrary("pthread");
    lib.linkLibCpp();
    b.installArtifact(lib);
}
