#include "executor.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "api.h"
#include "dependency_scan.h"

namespace module_builder {
namespace {

std::filesystem::path builder_output_path(const Context &ctx, const std::string &module_name) {
    return ctx.build_root / module_name / "builder_executable";
}

void ensure_directory(const std::filesystem::path &path) {
    std::filesystem::create_directories(path);
}

void build_builder(const Context &ctx, const std::string &module_name) {
    auto source = builder_source_path(ctx, module_name);
    auto output = builder_output_path(ctx, module_name);

    ensure_directory(output.parent_path());

    bool needs_build = !std::filesystem::exists(output) ||
                       std::filesystem::last_write_time(source) > std::filesystem::last_write_time(output);

    if (!needs_build) {
        return;
    }

    std::stringstream cmd;
    cmd << "g++ -std=c++17 -Isrc -o " << output << ' ' << source;
    std::cout << "Compiling builder for module " << module_name << "\n";
    if (std::system(cmd.str().c_str()) != 0) {
        throw std::runtime_error("Failed to compile builder for module: " + module_name);
    }
}

void topological_visit(const Context &ctx,
                       const std::string &module_name,
                       std::unordered_map<std::string, std::vector<std::string>> &graph,
                       std::unordered_set<std::string> &temporary,
                       std::unordered_set<std::string> &permanent,
                       std::vector<std::string> &order) {
    if (permanent.count(module_name)) {
        return;
    }
    if (temporary.count(module_name)) {
        throw std::runtime_error("Detected dependency cycle involving module: " + module_name);
    }

    temporary.insert(module_name);
    auto it = graph.find(module_name);
    if (it == graph.end()) {
        graph[module_name] = scan_dependencies(ctx, module_name);
        it = graph.find(module_name);
    }

    for (const auto &dep : it->second) {
        topological_visit(ctx, dep, graph, temporary, permanent, order);
    }

    temporary.erase(module_name);
    permanent.insert(module_name);
    order.push_back(module_name);
}

std::vector<std::string> compute_build_order(const Context &ctx, const std::string &root_module) {
    std::unordered_map<std::string, std::vector<std::string>> graph;
    std::unordered_set<std::string> temporary;
    std::unordered_set<std::string> permanent;
    std::vector<std::string> order;

    topological_visit(ctx, root_module, graph, temporary, permanent, order);
    return order;
}

void run_builder_executable(const Context &ctx, const std::string &module_name) {
    auto executable = builder_output_path(ctx, module_name);
    if (!std::filesystem::exists(executable)) {
        throw std::runtime_error("Builder executable missing for module: " + module_name);
    }

    std::stringstream cmd;
    cmd << executable << ' ' << module_name;

    std::cout << "Running builder for module " << module_name << "\n";
    if (std::system(cmd.str().c_str()) != 0) {
        throw std::runtime_error("Builder execution failed for module: " + module_name);
    }
}

}  // namespace

int execute(const Context &ctx) {
    auto order = compute_build_order(ctx, ctx.module_name);

    for (const auto &module : order) {
        build_builder(ctx, module);
    }

    run_builder_executable(ctx, ctx.module_name);
    return 0;
}

}  // namespace module_builder
