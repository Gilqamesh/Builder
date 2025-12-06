#include "api.h"

#include "executor.h"

std::filesystem::path artifact_path(
    const context_t& ctx,
    const std::string& module_name,
    artifact_kind_t kind
) {
    auto module_build_dir = ctx.build_root / module_name;
    switch (kind) {
        case artifact_kind_t::STATIC_LIBRARY:
            return module_build_dir / (module_name + ".static_library");
        case artifact_kind_t::SHARED_LIBRARY:
            return module_build_dir / (module_name + ".shared_library");
        case artifact_kind_t::EXECUTABLE:
        default:
            return module_build_dir / (module_name + ".executable");
    }
}

int build_module(const std::string& module_name) {
    auto ctx = make_context_for_module(module_name);
    return execute_build(ctx);
}
