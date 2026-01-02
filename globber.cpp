#include "globber.h"
#include "internal.h"

#include <format>

std::vector<std::string> globber_t::cpp_files_recursive(builder_ctx_t* ctx, const builder_api_t* api) {
    return cpp_files_recursive(api->source_dir(ctx), { BUILDER_PLUGIN_CPP });
}

std::vector<std::string> globber_t::cpp_files_recursive(const std::filesystem::path& dir, const std::unordered_set<std::string>& relative_exclude_files) {
    std::vector<std::string> source_files;

    std::error_code ec;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(dir, ec)) {
        const auto& path = entry.path();
        if (path.extension() != ".cpp") {
            continue ;
        }

        const auto& rel_filename = path.lexically_relative(dir);
        if (relative_exclude_files.contains(rel_filename)) {
            continue ;
        }

        source_files.push_back(rel_filename);
    }
    if (ec) {
        throw std::runtime_error(std::format("cpp_files_recursive: failed to recursively iterate source directory '{}': {}", dir.string(), ec.message()));
    }

    return source_files;
}
