#pragma once

#include <filesystem>
#include <string>

#include "context.h"

enum class artifact_kind_t { STATIC_LIBRARY, SHARED_LIBRARY, EXECUTABLE };

std::filesystem::path artifact_path(
    const context_t& ctx,
    const std::string& module_name,
    artifact_kind_t kind
);

int build_module(const std::string& module_name);
