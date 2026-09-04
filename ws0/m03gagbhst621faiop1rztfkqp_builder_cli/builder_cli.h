#ifndef M03GAGBHST621FAIOP1RZTFKQP_BUILDER_CLI_BUILDER_CLI_H
# define M03GAGBHST621FAIOP1RZTFKQP_BUILDER_CLI_BUILDER_CLI_H

# include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>

# include <vector>
# include <string>
# include <string_view>

namespace m03gagbhst621faiop1rztfkqp_builder_cli {

/**
 * @brief Builds a module's default CLI and replaces the current process with it.
 *
 * @param module The module whose default CLI should be invoked.
 * @param argc The number of additional module arguments in argv.
 * @param argv The additional arguments for the module invocation.
 *
 * Fails if building the CLI or replacing the process fails.
 */
[[noreturn]] void exec(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t module, int argc, const char* const* argv);

/**
 * @brief Builds a module's selected binary target and replaces the current process with it.
 *
 * @param module The module whose target should be invoked.
 * @param target The binary target to invoke.
 * @param argc The number of additional module arguments in argv.
 * @param argv The additional arguments for the module invocation.
 *
 * Fails if building the target or replacing the process fails.
 */
[[noreturn]] void exec(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t module, std::string_view target, int argc, const char* const* argv);

/**
 * @brief Builds a module's default CLI and replaces the current process with it.
 *
 * @param module The module whose default CLI should be invoked.
 * @param args The additional arguments for the module invocation.
 *
 * Fails if building the CLI or replacing the process fails.
 */
[[noreturn]] void exec(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t module, std::vector<std::string> args);

/**
 * @brief Builds a module's selected binary target and replaces the current process with it.
 *
 * @param module The module whose target should be invoked.
 * @param target The binary target to invoke.
 * @param args The additional arguments for the module invocation.
 *
 * Fails if building the target or replacing the process fails.
 */
[[noreturn]] void exec(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t module, std::string_view target, std::vector<std::string> args);

/**
 * @brief Builds a module's default CLI, invokes it, and waits for it to finish.
 *
 * @param module The module whose default CLI should be invoked.
 * @param argc The number of additional module arguments in argv.
 * @param argv The additional arguments for the module invocation.
 *
 * Fails if building the CLI, launching the process, waiting
 * for it, or a non-zero process result fails the checked wait.
 */
void create_and_wait_checked(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t module, int argc, const char* const* argv);

/**
 * @brief Builds a module's selected binary target, invokes it, and waits for it to finish.
 *
 * @param module The module whose target should be invoked.
 * @param target The binary target to invoke.
 * @param argc The number of additional module arguments in argv.
 * @param argv The additional arguments for the module invocation.
 *
 * Fails if building the target, launching the process,
 * waiting for it, or a non-zero process result fails the checked wait.
 */
void create_and_wait_checked(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t module, std::string_view target, int argc, const char* const* argv);

/**
 * @brief Builds a module's default CLI, invokes it, and waits for it to finish.
 *
 * @param module The module whose default CLI should be invoked.
 * @param args The additional arguments for the module invocation.
 *
 * Fails if building the CLI, launching the process, waiting
 * for it, or a non-zero process result fails the checked wait.
 */
void create_and_wait_checked(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t module, std::vector<std::string> args);

/**
 * @brief Builds a module's selected binary target, invokes it, and waits for it to finish.
 *
 * @param module The module whose target should be invoked.
 * @param target The binary target to invoke.
 * @param args The additional arguments for the module invocation.
 *
 * Fails if building the target, launching the process,
 * waiting for it, or a non-zero process result fails the checked wait.
 */
void create_and_wait_checked(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t module, std::string_view target, std::vector<std::string> args);

} // namespace m03gagbhst621faiop1rztfkqp_builder_cli

#endif // M03GAGBHST621FAIOP1RZTFKQP_BUILDER_CLI_BUILDER_CLI_H
