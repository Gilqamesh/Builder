#pragma once

#include <filesystem>
#include <string>

namespace module_builder {

struct Context {
    std::filesystem::path workspace_root;
    std::filesystem::path modules_root;
    std::filesystem::path build_root;
    std::string module_name;
    std::filesystem::path module_dir;
};

Context load_context(int argc, char **argv);

}  // namespace module_builder
