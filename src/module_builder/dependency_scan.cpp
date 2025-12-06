#include "dependency_scan.h"

#include <filesystem>
#include <fstream>
#include <regex>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

namespace module_builder {

std::filesystem::path builder_source_path(const Context &ctx, const std::string &module_name) {
    return ctx.modules_root / module_name / "builder.cpp";
}

std::vector<std::string> scan_dependencies(const Context &ctx, const std::string &module_name) {
    auto path = builder_source_path(ctx, module_name);
    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("Missing builder.cpp for module: " + module_name);
    }

    std::ifstream input(path);
    if (!input.is_open()) {
        throw std::runtime_error("Unable to open builder.cpp for module: " + module_name);
    }

    std::unordered_set<std::string> unique;
    std::string line;
    std::regex pattern(R"(#\s*include\s*[<\"]module/([^/]+)/[^>\"]+[>\"])");
    while (std::getline(input, line)) {
        std::smatch match;
        if (std::regex_search(line, match, pattern) && match.size() > 1) {
            unique.insert(match[1].str());
        }
    }

    return std::vector<std::string>(unique.begin(), unique.end());
}

}  // namespace module_builder
