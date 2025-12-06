#include "dependency_scan.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>
#include <string>

namespace module_builder {

std::filesystem::path builder_source_path(
    const context_t& ctx,
    const std::string& module_name) {
    return ctx.modules_root / module_name / "builder.cpp";
}

namespace {

bool is_module_present(const context_t& ctx, const std::string& name) {
    std::filesystem::path dir = ctx.modules_root / name;
    std::filesystem::path builder = dir / "builder.cpp";
    return std::filesystem::exists(dir) && std::filesystem::is_directory(dir) &&
           std::filesystem::exists(builder);
}

std::string extract_include_target(const std::string& line) {
    std::string::size_type pos = line.find("#include");
    if (pos == std::string::npos) {
        return {};
    }

    auto start = line.find_first_of("\"<", pos);
    auto end = line.find_first_of(">\"", start + 1);
    if (start == std::string::npos || end == std::string::npos || end <= start + 1) {
        return {};
    }

    std::string path = line.substr(start + 1, end - start - 1);
    auto slash = path.find('/');
    if (slash == std::string::npos) {
        return {};
    }
    return path.substr(0, slash);
}

}

std::vector<std::string> scan_builder_dependencies(
    const context_t& ctx,
    const std::string& module_name) {
    std::filesystem::path source = builder_source_path(ctx, module_name);
    if (!std::filesystem::exists(source)) {
        std::cerr << "missing builder.cpp for module '" << module_name << "' at " << source << "\n";
        std::exit(1);
    }

    std::ifstream in(source);
    if (!in) {
        std::cerr << "failed to open builder.cpp for module '" << module_name << "'\n";
        std::exit(1);
    }

    std::set<std::string> deps;
    std::string line;
    while (std::getline(in, line)) {
        std::string candidate = extract_include_target(line);
        if (candidate.empty()) {
            continue;
        }
        if (is_module_present(ctx, candidate)) {
            deps.insert(candidate);
        }
    }

    return std::vector<std::string>(deps.begin(), deps.end());
}

}  // namespace module_builder

