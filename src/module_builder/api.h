#pragma once

#include <filesystem>
#include <string>

#include "context.h"

namespace module_builder {

enum class artifact_kind_t {
    STATIC_LIBRARY,
    SHARED_LIBRARY,
    EXECUTABLE,
};

inline std::filesystem::path artifact_path(
    const context_t& ctx,
    const std::string& module_name,
    artifact_kind_t kind) {
    std::filesystem::path base = ctx.build_root / module_name;
    switch (kind) {
        case artifact_kind_t::STATIC_LIBRARY:
            return base / (module_name + ".static_library");
        case artifact_kind_t::SHARED_LIBRARY:
            return base / (module_name + ".shared_library");
        case artifact_kind_t::EXECUTABLE:
            return base / (module_name + ".executable");
    }
    return {};
}

int build_module(const std::string& module_name);

}  // namespace module_builder

