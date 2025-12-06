#include "api.h"

#include <stdexcept>
#include <string>

#include "executor.h"

namespace module_builder {

std::filesystem::path artifact_path(const Context &ctx, const std::string &module_name, ArtifactKind kind) {
    std::filesystem::path base = ctx.build_root / module_name / module_name;
    switch (kind) {
        case ArtifactKind::StaticLibrary:
            return base.concat(".static_library");
        case ArtifactKind::SharedLibrary:
            return base.concat(".shared_library");
        case ArtifactKind::Executable:
            return base.concat(".executable");
    }
    throw std::runtime_error("Unknown artifact kind");
}

int build(const Context &ctx) {
    return execute(ctx);
}

}  // namespace module_builder
