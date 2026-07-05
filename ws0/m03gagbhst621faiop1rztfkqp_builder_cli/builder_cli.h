#ifndef M03GAGBHST621FAIOP1RZTFKQP_BUILDER_CLI_H
# define M03GAGBHST621FAIOP1RZTFKQP_BUILDER_CLI_H

# include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>
# include <m03gagbhsvr0m5w15urj0o291m_process/process.h>

# include <vector>

namespace m03gagbhst621faiop1rztfkqp_builder_cli {

/**
 * Builds a module's default CLI and replaces the current process with it.
 */
[[noreturn]] void run(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t module, const std::vector<m03gagbhsvr0m5w15urj0o291m_process::process_arg_t>& args);

} // namespace m03gagbhst621faiop1rztfkqp_builder_cli

#endif // M03GAGBHST621FAIOP1RZTFKQP_BUILDER_CLI_H
