const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // fetch commit dependency
    const upstream = b.dependency("upstream", .{});
    const raylib_src = upstream.path("src");

    const lib = b.addLibrary(.{
        .name = "raylib",
        .linkage = .static,
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
        }),
    });

    const files = [_][]const u8{
        "rcore.c",
        "rshapes.c",
        "rtextures.c",
        "rtext.c",
        "rmodels.c",
        "raudio.c",
        "rglfw.c",
    };

    // Zig 0.15: addCSourceFile takes a LazyPath (NOT a string)
    for (files) |fname| {
        lib.root_module.addCSourceFile(.{
            .file = raylib_src.path(b, fname),
            .flags = &.{ "-std=c99" },
        });
    }

    lib.root_module.addIncludePath(raylib_src);

    lib.linkLibC();
    lib.linkSystemLibrary("m");
    lib.linkSystemLibrary("GL");
    lib.linkSystemLibrary("dl");
    lib.linkSystemLibrary("rt");
    lib.linkSystemLibrary("pthread");
    lib.linkSystemLibrary("X11");

    b.installArtifact(lib);
}
