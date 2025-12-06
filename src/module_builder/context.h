#pragma once

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

namespace module_builder {

struct context_t {
    std::filesystem::path workspace_root;
    std::filesystem::path modules_root;
    std::filesystem::path build_root;
    std::string module_name;
    std::filesystem::path module_dir;
};

inline context_t make_context_for_module(const std::string& module_name) {
    context_t ctx;
    ctx.workspace_root = std::filesystem::current_path();
    ctx.modules_root = ctx.workspace_root / "src";
    ctx.build_root = ctx.workspace_root / "build";
    ctx.module_name = module_name;
    ctx.module_dir = ctx.modules_root / module_name;

    if (!std::filesystem::exists(ctx.module_dir) ||
        !std::filesystem::is_directory(ctx.module_dir)) {
        std::cerr << "module directory does not exist: " << ctx.module_dir << "\n";
        std::exit(1);
    }

    return ctx;
}

inline context_t load_context_from_argv(int argc, char** argv) {
    if (argc < 5) {
        std::cerr << "expected arguments: <workspace_root> <modules_root> <build_root> <module_name>" << std::endl;
        std::exit(1);
    }

    context_t ctx;
    ctx.workspace_root = std::filesystem::path(argv[1]);
    ctx.modules_root = std::filesystem::path(argv[2]);
    ctx.build_root = std::filesystem::path(argv[3]);
    ctx.module_name = argv[4];
    ctx.module_dir = ctx.modules_root / ctx.module_name;

    if (!std::filesystem::exists(ctx.module_dir) ||
        !std::filesystem::is_directory(ctx.module_dir)) {
        std::cerr << "invalid module directory: " << ctx.module_dir << "\n";
        std::exit(1);
    }

    return ctx;
}

}  // namespace module_builder

