#include "context.h"

#include <filesystem>
#include <stdexcept>

namespace module_builder {

Context load_context(int argc, char **argv) {
    if (argc < 2) {
        throw std::runtime_error("Expected module name argument");
    }

    Context ctx;
    ctx.workspace_root = std::filesystem::current_path();
    ctx.modules_root = ctx.workspace_root / "src";
    ctx.build_root = ctx.workspace_root / "build";
    ctx.module_name = argv[1];
    ctx.module_dir = ctx.modules_root / ctx.module_name;
    return ctx;
}

}  // namespace module_builder
