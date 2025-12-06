#include "context.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>

context_t make_context_for_module(const std::string& module_name) {
    context_t ctx;
    ctx.workspace_root = std::filesystem::current_path();
    ctx.modules_root = ctx.workspace_root / "src";
    ctx.build_root = ctx.workspace_root / "build";
    ctx.module_name = module_name;
    ctx.module_dir = ctx.modules_root / module_name;

    auto builder_path = ctx.module_dir / "builder.cpp";
    if (!std::filesystem::exists(builder_path)) {
        std::cerr << "builder.cpp not found for module: " << module_name << std::endl;
        std::exit(1);
    }

    return ctx;
}

context_t load_context_from_argv(int argc, char** argv) {
    (void)argc;
    context_t ctx;
    ctx.workspace_root = std::filesystem::path(argv[1]);
    ctx.modules_root = std::filesystem::path(argv[2]);
    ctx.build_root = std::filesystem::path(argv[3]);
    ctx.module_name = argv[4];
    ctx.module_dir = ctx.modules_root / ctx.module_name;
    return ctx;
}
