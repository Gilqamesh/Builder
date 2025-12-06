#include "dependency_scan.h"

#include <fstream>
#include <string>
#include <unordered_set>

std::filesystem::path builder_source_path(const context_t& ctx, const std::string& module_name) {
    return ctx.modules_root / module_name / "builder.cpp";
}

std::vector<std::string> scan_builder_dependencies(
    const context_t& ctx,
    const std::string& module_name
) {
    std::vector<std::string> dependencies;
    std::unordered_set<std::string> seen;

    auto source_path = builder_source_path(ctx, module_name);
    std::ifstream input(source_path);
    if (!input.is_open()) {
        return dependencies;
    }

    std::string line;
    while (std::getline(input, line)) {
        auto pos = line.find("#include");
        if (pos == std::string::npos) {
            continue;
        }

        size_t start = line.find('<', pos);
        size_t end = std::string::npos;
        if (start != std::string::npos) {
            end = line.find('>', start + 1);
        } else {
            start = line.find('"', pos);
            if (start != std::string::npos) {
                end = line.find('"', start + 1);
            }
        }

        if (start == std::string::npos || end == std::string::npos || end <= start + 1) {
            continue;
        }

        std::string include_path = line.substr(start + 1, end - start - 1);
        auto slash_pos = include_path.find('/');
        std::string dependency_module = include_path.substr(0, slash_pos);

        auto dependency_builder = ctx.modules_root / dependency_module / "builder.cpp";
        if (std::filesystem::exists(dependency_builder) && seen.insert(dependency_module).second) {
            dependencies.push_back(dependency_module);
        }
    }

    return dependencies;
}
