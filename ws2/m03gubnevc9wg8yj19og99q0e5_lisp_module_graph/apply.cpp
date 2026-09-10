#include "apply.h"

#include <m03gubnevc9z8dwzxigmj54y25_lisp_module_command/module_command.h>
#include <m03gubnevca18el75zd2vx8qxh_language/language.h>
#include <m03gagbhtahg11wzn32idilzte_module_graph/module_graph.h>

#include <m03gubnevca0u4aqlfbtuz06ix_lisp_runtime/runtime.h>

#include <format>
#include <stdexcept>

namespace m03gubnevc9wg8yj19og99q0e5_lisp_module_graph {


using namespace m03gagbhtahg11wzn32idilzte_module_graph;

static m03gagbhsnusi43zogoacgj2ez_filesystem::path_t path_arg(const m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t& value) {
    if (m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::is_string(value)) {
        return m03gagbhsnusi43zogoacgj2ez_filesystem::path_t(m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_string(value));
    }

    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_path(value);
}

m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t apply(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    if (args.size() != 2) {
        throw std::runtime_error(std::format("m03gagbhtahg11wzn32idilzte_module_graph::apply: expected 2 arguments, got {}", args.size()));
    }

    const auto invocation_context = m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::invocation_context();
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t workspace_graph(
        invocation_context.workspace_root,
        invocation_context.artifact_root
    );
    auto& target_module = m03gubnevc9z8dwzxigmj54y25_lisp_module_command::resolve_module(workspace_graph, m03gubnevca18el75zd2vx8qxh_language::as_module_name(args[0]));

    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::path_value(render_svg(
        workspace_graph,
        target_module,
        path_arg(args[1])
    ));
}


extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__apply(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    return m03gubnevc9wg8yj19og99q0e5_lisp_module_graph::apply(args);
}

} // namespace m03gubnevc9wg8yj19og99q0e5_lisp_module_graph
