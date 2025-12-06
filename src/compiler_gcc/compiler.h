#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "module_builder/api.h"
#include "module_builder/context.h"

namespace compiler_gcc {

struct BuildConfig {
    std::vector<std::string> include_dirs;
};

std::filesystem::path build_static_library(const module_builder::Context &ctx,
                                           const std::string &module_name,
                                           const std::vector<std::filesystem::path> &sources,
                                           const std::vector<std::string> &deps,
                                           const BuildConfig &config);

}  // namespace compiler_gcc
