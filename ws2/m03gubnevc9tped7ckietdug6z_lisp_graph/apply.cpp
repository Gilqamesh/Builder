#include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>
#include <m03gubnevca0u4aqlfbtuz06ix_lisp_runtime/runtime.h>

#include <format>
#include <stdexcept>
#include <vector>

namespace m03gubnevc9tped7ckietdug6z_lisp_graph {


void require_arg_count(
    const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args,
    std::size_t expected,
    std::string_view name
) {
    if (args.size() != expected) {
        throw std::runtime_error(std::format(
            "m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::{}: expected {} arguments, got {}",
            name,
            expected,
            args.size()
        ));
    }
}

m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t graph_capability(std::string_view name) {
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::capability_value(m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("graph"), name);
}

m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t workspace_graph() {
    const auto invocation_context = m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::invocation_context();
    return m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t(
        invocation_context.workspace_root,
        invocation_context.artifact_root
    );
}


extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__apply(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    require_arg_count(args, 0, "apply");
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::record_value({
        { "artifact_root", graph_capability("artifact_root") },
        { "kernel_module", graph_capability("kernel_module") },
        { "modules", graph_capability("modules") },
        { "workspace_root", graph_capability("workspace_root") }
    });
}

extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__capability__artifact_root(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    require_arg_count(args, 0, "artifact_root");
    auto graph = workspace_graph();
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::path_value(graph.artifact_root());
}

extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__capability__kernel_module(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    require_arg_count(args, 0, "kernel_module");
    auto graph = workspace_graph();
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::string_value("m03gagbhst621faiop1rztfkqp_builder_cli");
}

extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__capability__modules(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    require_arg_count(args, 0, "modules");
    auto graph = workspace_graph();
    std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t> modules;
    for (const auto& module_name : graph.module_names()) {
        modules.push_back(m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::string_value(module_name.unique_name()));
    }

    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::list_value(modules);
}

extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__capability__workspace_root(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    require_arg_count(args, 0, "workspace_root");
    auto graph = workspace_graph();
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::path_value(graph.root());
}

} // namespace m03gubnevc9tped7ckietdug6z_lisp_graph
