const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const lib = b.addStaticLibrary(.{
        .name = "imgui",
        .target = target,
        .optimize = optimize,
    });
    const sources = &[_]std.Build.LazyPath{
        b.path("imgui.cpp"),
        b.path("imgui_demo.cpp"),
        b.path("imgui_draw.cpp"),
        b.path("imgui_tables.cpp"),
        b.path("imgui_widgets.cpp"),
        b.path("backends/imgui_impl_glfw.cpp"),
        b.path("backends/imgui_impl_opengl3.cpp"),
    };
    lib.addCSourceFiles(.{ .files = sources, .flags = &.{ "-std=c++23" } });
    lib.addIncludePath(b.path("."));
    lib.addIncludePath(b.path("backends"));
    lib.addIncludePath(b.path("misc"));
    lib.linkSystemLibrary("glfw");
    lib.linkSystemLibrary("GL");
    lib.linkSystemLibrary("dl");
    lib.linkSystemLibrary("pthread");
    lib.linkLibCpp();
    b.installArtifact(lib);
}
