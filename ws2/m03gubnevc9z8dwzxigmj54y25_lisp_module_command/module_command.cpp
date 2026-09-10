#include "module_command.h"
#include "helpers.h"

#include <m03gagbhsujjf63n0w3r2w4q6h_build_phases/build_phases.h>
#include <m03gagbhsx4j5z28bqkac3dhhh_shared_library/shared_library.h>

#include <algorithm>
#include <cctype>
#include <format>
#include <stdexcept>

namespace m03gubnevc9z8dwzxigmj54y25_lisp_module_command {

m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& resolve_module(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& workspace_graph, const m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t& name) {
    const auto module_names = workspace_graph.module_names();
    for (const auto& module_name : module_names) {
        if (module_name.unique_name() == name.string()) {
            return *workspace_graph.discover_module(module_name);
        }
    }
    std::string logical_name = name.string();
    if (logical_name == "graph") { logical_name = "workspace_graph"; }
    if (logical_name == "compiler") { logical_name = "cxx_toolchain"; }
    if (logical_name == "phase") { logical_name = "build_phases"; }
    if (logical_name == "kernel") { logical_name = "builder_cli"; }
    if (logical_name == "runtime") { logical_name = "lisp_runtime"; }
    for (const auto& friendly_name : {logical_name, "lisp_" + name.string()}) {
        m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t* selected = nullptr;
        for (const auto& module_name : module_names) {
            if (module_name.friendly_name() == friendly_name) {
                if (selected != nullptr) {
                    throw std::runtime_error(std::format("Lisp module name '{}' is ambiguous; use its complete identity", name));
                }
                selected = workspace_graph.discover_module(module_name);
            }
        }
        if (selected != nullptr) {
            return *selected;
        }
    }
    throw std::runtime_error(std::format("Lisp module '{}' was not found", name));
}

std::vector<std::string> completion_names(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& workspace_graph) {
    std::vector<std::string> names;
    for (const auto& module_name : workspace_graph.module_names()) {
        names.push_back(module_name.unique_name());
        auto friendly_name = module_name.friendly_name();
        names.push_back(friendly_name);
        if (friendly_name.starts_with("lisp_")) {
            names.push_back(friendly_name.substr(5));
        }
    }
    std::ranges::sort(names);
    names.erase(std::unique(names.begin(), names.end()), names.end());
    return names;
}

m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t apply(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& workspace_graph, m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t module, const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    auto& target = resolve_module(workspace_graph, module);
    auto loader = load_module(target);
    using function_t = m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t (*)(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>&);
    const function_t function = loader.resolve("module__apply");
    return function(args);
}

m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t apply_capability(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& workspace_graph, m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t module, std::string_view capability, const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    if (capability.empty() || !(std::isalpha(static_cast<unsigned char>(capability.front())) || capability.front() == '_') || !std::ranges::all_of(capability, [](unsigned char ch) { return std::isalnum(ch) || ch == '_'; })) {
        throw std::runtime_error(std::format("Lisp capability '{}' is not a native symbol name", capability));
    }
    auto& target = resolve_module(workspace_graph, module);
    auto loader = load_module(target);
    using function_t = m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t (*)(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>&);
    const function_t function = loader.resolve(std::format("module__capability__{}", capability).c_str());
    return function(args);
}

} // namespace m03gubnevc9z8dwzxigmj54y25_lisp_module_command
