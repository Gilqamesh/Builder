#ifndef M03GAGBHST621FAIOP1RZTFKQP_BUILDER_CLI_H
# define M03GAGBHST621FAIOP1RZTFKQP_BUILDER_CLI_H

# include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>

# include <vector>
# include <span>
# include <string_view>

namespace m03gagbhst621faiop1rztfkqp_builder_cli {

/**
 * Builds a module's default CLI and replaces the current process with it.
 */
[[noreturn]] void exec(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t module, std::span<const std::string_view> args);

/**
 * Builds a module's default CLI and waits for it to finish.
 * Throws std::runtime_error if the process exits with a non-zero status.
 */
void create_and_wait_checked(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t module, std::span<const std::string_view> args);

} // namespace m03gagbhst621faiop1rztfkqp_builder_cli

#endif // M03GAGBHST621FAIOP1RZTFKQP_BUILDER_CLI_H
