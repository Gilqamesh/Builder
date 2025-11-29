const std = @import("std");

pub fn build(b: *std.Build) void {
    const upstream = b.dependency("upstream", .{});
    _ = upstream;
}
