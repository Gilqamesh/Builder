#pragma once

#include <filesystem>
#include <string>

struct context_t {
    std::filesystem::path workspace_root;
    std::filesystem::path modules_root;
    std::filesystem::path build_root;
    std::string module_name;
    std::filesystem::path module_dir;
};

context_t make_context_for_module(const std::string& module_name);

context_t load_context_from_argv(int argc, char** argv);
