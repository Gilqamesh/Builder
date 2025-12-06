#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "context.h"

std::filesystem::path builder_source_path(const context_t& ctx, const std::string& module_name);

std::vector<std::string> scan_builder_dependencies(
    const context_t& ctx,
    const std::string& module_name
);
