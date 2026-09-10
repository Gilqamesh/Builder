#include <m03gubnevc9z8dwzxigmj54y25_lisp_module_command/module_command.h>
#include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>
#include <m03gubnevca0u4aqlfbtuz06ix_lisp_runtime/runtime.h>

#include <format>
#include <stdexcept>
#include <vector>

namespace m03gubnevc9z8dwzxigmj54y25_lisp_module_command {


void require_arg_count(
    const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args,
    std::size_t expected,
    std::string_view name
) {
    if (args.size() != expected) {
        throw std::runtime_error(std::format(
            "m03gubnevc9z8dwzxigmj54y25_lisp_module_command::{}: expected {} arguments, got {}",
            name,
            expected,
            args.size()
        ));
    }
}

m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module_command_capability(std::string_view name) {
    return m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::capability_value(m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("module_command"), name);
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
        { "apply", module_command_capability("apply") },
        { "apply_capability", module_command_capability("apply_capability") }
    });
}

extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__capability__apply(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    require_arg_count(args, 2, "apply");
    auto graph = workspace_graph();
    return m03gubnevc9z8dwzxigmj54y25_lisp_module_command::apply(
        graph,
        m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_string(args[0])),
        m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_list(args[1])
    );
}

extern "C" m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t module__capability__apply_capability(const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args) {
    require_arg_count(args, 3, "apply_capability");
    auto graph = workspace_graph();
    return m03gubnevc9z8dwzxigmj54y25_lisp_module_command::apply_capability(
        graph,
        m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_string(args[0])),
        m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_string(args[1]),
        m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::as_list(args[2])
    );
}

} // namespace m03gubnevc9z8dwzxigmj54y25_lisp_module_command
