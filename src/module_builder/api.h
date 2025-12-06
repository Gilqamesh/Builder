#pragma once

#include <filesystem>
#include <string>

#include "context.h"

namespace module_builder {

enum class ArtifactKind {
    StaticLibrary,
    SharedLibrary,
    Executable,
};

std::filesystem::path artifact_path(const Context &ctx, const std::string &module_name, ArtifactKind kind);

int build(const Context &ctx);

}  // namespace module_builder
