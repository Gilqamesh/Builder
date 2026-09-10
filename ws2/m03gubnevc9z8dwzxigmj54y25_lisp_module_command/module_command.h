#ifndef M03GUBNEVC9Z8DWZXIGMJ54Y25_LISP_MODULE_COMMAND_MODULE_COMMAND_H
# define M03GUBNEVC9Z8DWZXIGMJ54Y25_LISP_MODULE_COMMAND_MODULE_COMMAND_H

# include <m03gubnevca0u4aqlfbtuz06ix_lisp_runtime/runtime.h>
# include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>

# include <string_view>
# include <vector>

namespace m03gubnevc9z8dwzxigmj54y25_lisp_module_command {

/** @brief Resolves a complete module identity or an unambiguous Lisp module name. */
m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& resolve_module(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& workspace_graph, const m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t& name);
std::vector<std::string> completion_names(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& workspace_graph);
/** @brief Invokes a module's native `module__apply` export with runtime arguments. */
m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t apply(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& workspace_graph, m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t module, const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args);
/** @brief Invokes a named native capability with runtime arguments. */
m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t apply_capability(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& workspace_graph, m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t module, std::string_view capability, const std::vector<m03gubnevca0u4aqlfbtuz06ix_lisp_runtime::value_t>& args);

} // namespace m03gubnevc9z8dwzxigmj54y25_lisp_module_command

#endif // M03GUBNEVC9Z8DWZXIGMJ54Y25_LISP_MODULE_COMMAND_MODULE_COMMAND_H
