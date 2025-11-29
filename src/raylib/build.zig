const std = @import("std");

pub fn build(b: *std.Build) void {
    // Pull raylib upstream and run its own build.zig
    const upstream = b.dependency("upstream", .{});
    _ = upstream;
}
