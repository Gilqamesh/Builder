#include "executor.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "dependency_scan.h"

namespace {
std::unordered_map<std::string, std::vector<std::string>> build_dependency_graph(
    const context_t& ctx,
    const std::string& module_name) {
    std::unordered_map<std::string, std::vector<std::string>> graph;
    std::vector<std::string> stack{module_name};

    while (!stack.empty()) {
        auto current = stack.back();
        stack.pop_back();
        if (graph.find(current) != graph.end()) {
            continue;
        }

        auto deps = scan_builder_dependencies(ctx, current);
        graph[current] = deps;
        for (const auto& dep : deps) {
            if (graph.find(dep) == graph.end()) {
                stack.push_back(dep);
            }
        }
    }

    return graph;
}

bool topo_visit(
    const std::string& module,
    const std::unordered_map<std::string, std::vector<std::string>>& graph,
    std::unordered_map<std::string, int>& state,
    std::vector<std::string>& order) {
    auto it = state.find(module);
    if (it != state.end()) {
        if (it->second == 1) {
            std::cerr << "Cycle detected involving module: " << module << std::endl;
            return false;
        }
        if (it->second == 2) {
            return true;
        }
    }

    state[module] = 1;
    auto deps_it = graph.find(module);
    if (deps_it != graph.end()) {
        for (const auto& dep : deps_it->second) {
            if (!topo_visit(dep, graph, state, order)) {
                return false;
            }
        }
    }

    state[module] = 2;
    order.push_back(module);
    return true;
}
}

int execute_build(const context_t& root_ctx) {
    auto graph = build_dependency_graph(root_ctx, root_ctx.module_name);

    std::vector<std::string> order;
    std::unordered_map<std::string, int> state;
    if (!topo_visit(root_ctx.module_name, graph, state, order)) {
        return 1;
    }

    for (auto it = graph.begin(); it != graph.end(); ++it) {
        const auto& module = it->first;
        if (state.find(module) == state.end()) {
            if (!topo_visit(module, graph, state, order)) {
                return 1;
            }
        }
    }

    for (const auto& module : order) {
        auto builder_exe = root_ctx.build_root / module / (module + ".builder");
        auto builder_src = builder_source_path(root_ctx, module);
        bool need_rebuild = !std::filesystem::exists(builder_exe) ||
                            std::filesystem::last_write_time(builder_src) >
                                std::filesystem::last_write_time(builder_exe);

        if (need_rebuild) {
            std::filesystem::create_directories(builder_exe.parent_path());
            std::stringstream compile_cmd;
            compile_cmd << "g++ -std=c++20 -Isrc -o " << builder_exe << " " << builder_src;
            int compile_result = std::system(compile_cmd.str().c_str());
            if (compile_result != 0) {
                return compile_result;
            }
        }

        std::stringstream run_cmd;
        run_cmd << builder_exe << " " << root_ctx.workspace_root << " "
                << root_ctx.modules_root << " " << root_ctx.build_root << " "
                << module;

        int run_result = std::system(run_cmd.str().c_str());
        if (run_result != 0) {
            return run_result;
        }
    }

    return 0;
}
