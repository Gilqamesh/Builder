#pragma once

#include <string>
#include <vector>

#include "context.h"

namespace module_builder {

std::filesystem::path builder_source_path(const Context &ctx, const std::string &module_name);
std::vector<std::string> scan_dependencies(const Context &ctx, const std::string &module_name);

}  // namespace module_builder
