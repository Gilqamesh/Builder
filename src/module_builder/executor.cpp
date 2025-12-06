#include "executor.h"

#include <cstdlib>
#include <iostream>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "dependency_scan.h"

namespace module_builder {

namespace {

struct graph_state_t {
    std::unordered_map<std::string, std::vector<std::string>> edges;
    std::vector<std::string> ordered;
    std::unordered_set<std::string> visiting;
    std::unordered_set<std::string> visited;
};

context_t make_module_context(const context_t& root_ctx, const std::string& module_name) {
    context_t ctx = root_ctx;
    ctx.module_name = module_name;
    ctx.module_dir = ctx.modules_root / module_name;
    return ctx;
}

void dfs_dependencies(const context_t& root_ctx, const std::string& module_name, graph_state_t& graph) {
    if (graph.visiting.count(module_name)) {
        std::cerr << "cycle detected involving module '" << module_name << "'" << std::endl;
        std::exit(1);
    }
    if (graph.visited.count(module_name)) {
        return;
    }

    graph.visiting.insert(module_name);
    context_t ctx = make_module_context(root_ctx, module_name);
    std::vector<std::string> deps = scan_builder_dependencies(ctx, module_name);
    graph.edges[module_name] = deps;

    for (const std::string& dep : deps) {
        dfs_dependencies(root_ctx, dep, graph);
    }

    graph.visiting.erase(module_name);
    graph.visited.insert(module_name);
    graph.ordered.push_back(module_name);
}

std::vector<std::string> compute_build_order(const context_t& root_ctx) {
    graph_state_t graph;
    dfs_dependencies(root_ctx, root_ctx.module_name, graph);
    return graph.ordered;
}

bool should_rebuild(const std::filesystem::path& source, const std::filesystem::path& exe) {
    if (!std::filesystem::exists(exe)) {
        return true;
    }
    std::error_code ec;
    auto source_time = std::filesystem::last_write_time(source, ec);
    if (ec) {
        return true;
    }
    auto exe_time = std::filesystem::last_write_time(exe, ec);
    if (ec) {
        return true;
    }
    return source_time > exe_time;
}

void build_builder_executable(const context_t& ctx, const std::string& module_name) {
    context_t module_ctx = make_module_context(ctx, module_name);
    std::filesystem::path source = builder_source_path(module_ctx, module_name);
    std::filesystem::path output_dir = ctx.build_root / module_name;
    std::filesystem::create_directories(output_dir);
    std::filesystem::path builder_exe = output_dir / (module_name + ".builder");

    if (!should_rebuild(source, builder_exe)) {
        return;
    }

    std::string command = "g++ -std=c++20 -Wall -Isrc -o " + builder_exe.string() + " " + source.string();
    std::cout << "[build] " << module_name << " builder" << std::endl;
    int rc = std::system(command.c_str());
    if (rc != 0) {
        std::cerr << "failed to compile builder for module '" << module_name << "'" << std::endl;
        std::exit(1);
    }
}

int run_builder(const context_t& ctx) {
    std::filesystem::path builder_exe = ctx.build_root / ctx.module_name / (ctx.module_name + ".builder");
    std::string command = builder_exe.string() + " " + ctx.workspace_root.string() + " " +
                          ctx.modules_root.string() + " " + ctx.build_root.string() + " " + ctx.module_name;
    std::cout << "[run] " << ctx.module_name << " builder" << std::endl;
    return std::system(command.c_str());
}

}  // namespace

int execute_build(const context_t& root_ctx) {
    std::vector<std::string> order = compute_build_order(root_ctx);

    for (const std::string& module_name : order) {
        build_builder_executable(root_ctx, module_name);
    }

    return run_builder(root_ctx);
}

}  // namespace module_builder

