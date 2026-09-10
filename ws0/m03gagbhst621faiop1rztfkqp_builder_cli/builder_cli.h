#ifndef M03GAGBHST621FAIOP1RZTFKQP_BUILDER_CLI_BUILDER_CLI_H
# define M03GAGBHST621FAIOP1RZTFKQP_BUILDER_CLI_BUILDER_CLI_H

# include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>

# include <vector>
# include <string>
# include <string_view>

/**
 * @brief Builds module binary targets and invokes them through Builder's bootstrap handling.
 *
 * Roots and process-environment updates follow
 * m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::invocation_context(). The configured
 * workspace must contain the bootstrap seed and requested module. Overloads
 * without target select `cli`; an explicit target names a published binary.
 * Use a non-empty filename other than `.` or `..`, with no path separators.
 *
 * All argument lists exclude the executable name: Builder supplies argv[0].
 * Arguments are literal strings, without shell splitting or expansion, and must
 * not contain embedded null characters. The target runs in its installed
 * executable's parent directory. For argc/argv overloads, argc must be
 * non-negative and argv must designate a valid range of argc non-null,
 * null-terminated strings. The range is copied during the call; it need not have
 * an extra null sentinel. From main(), pass `argc - 1, argv + 1` to forward only
 * additional arguments, or use an empty vector when there are none.
 *
 * Build/discovery failures propagate. Process setup, launch, and checked-wait
 * failures throw std::runtime_error; invalid command strings can throw
 * std::invalid_argument. exec() can throw despite [[noreturn]]: that attribute
 * rules out normal return. Successful exec() replaces the process without
 * unwinding the caller's automatic objects. If process replacement fails after
 * changing the working directory, that process-wide change is not rolled back.
 *
 * Given a module that publishes a `check` target accepting these arguments,
 * run it as a child and then transfer the current process to the same target:
 * @code{.cpp}
 * #include <m03gagbhst621faiop1rztfkqp_builder_cli/builder_cli.h>
 * #include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>
 *
 * #include <string>
 * #include <vector>
 *
 * namespace builder = m03gagbhst621faiop1rztfkqp_builder_cli;
 * namespace graph = m03gagbhsp2drqq3gkop8pzfrm_workspace_graph;
 *
 * void invoke_checks(const graph::module_name_t& module_name) {
 *     const std::vector<std::string> arguments { "--label", "two words" };
 *     builder::create_and_wait_checked(module_name, "check", arguments);
 *     // Reached only after a normal exit with status 0; failures throw.
 *     builder::exec(module_name, "check", arguments);
 *     // No normal continuation; exceptions can be caught by the caller.
 * }
 * @endcode
 */
namespace m03gagbhst621faiop1rztfkqp_builder_cli {

/**
 * @brief Builds a module's default CLI and replaces the current process with it.
 *
 * @param module The module whose default CLI should be invoked.
 * @param argc The number of additional module arguments in argv.
 * @param argv The additional arguments, excluding the executable name; see the namespace contract.
 *
 * Build and process-replacement failures throw. Success does not return.
 */
[[noreturn]] void exec(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t module, int argc, const char* const* argv);

/**
 * @brief Builds a module's selected binary target and replaces the current process with it.
 *
 * @param module The module whose target should be invoked.
 * @param target The binary target to invoke.
 * @param argc The number of additional module arguments in argv.
 * @param argv The additional arguments, excluding the executable name; see the namespace contract.
 *
 * Build and process-replacement failures throw. Success does not return.
 */
[[noreturn]] void exec(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t module, std::string_view target, int argc, const char* const* argv);

/**
 * @brief Builds a module's default CLI and replaces the current process with it.
 *
 * @param module The module whose default CLI should be invoked.
 * @param args Literal additional arguments, excluding the executable name.
 *
 * Build and process-replacement failures throw. Success does not return.
 */
[[noreturn]] void exec(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t module, std::vector<std::string> args);

/**
 * @brief Builds a module's selected binary target and replaces the current process with it.
 *
 * @param module The module whose target should be invoked.
 * @param target The binary target to invoke.
 * @param args Literal additional arguments, excluding the executable name.
 *
 * Build and process-replacement failures throw. Success does not return.
 */
[[noreturn]] void exec(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t module, std::string_view target, std::vector<std::string> args);

/**
 * @brief Builds a module's default CLI, invokes it, and waits for it to finish.
 *
 * @param module The module whose default CLI should be invoked.
 * @param argc The number of additional module arguments in argv.
 * @param argv The additional arguments, excluding the executable name; see the namespace contract.
 *
 * Returns normally only after status 0. Build failures propagate; launch/wait
 * failures, nonzero exit status, or signal termination throw. Foreground-terminal
 * behavior follows m03gagbhsvr0m5w15urj0o291m_process::create_and_wait_foreground_checked().
 */
void create_and_wait_checked(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t module, int argc, const char* const* argv);

/**
 * @brief Builds a module's selected binary target, invokes it, and waits for it to finish.
 *
 * @param module The module whose target should be invoked.
 * @param target The binary target to invoke.
 * @param argc The number of additional module arguments in argv.
 * @param argv The additional arguments, excluding the executable name; see the namespace contract.
 *
 * Returns normally only after status 0. Build failures propagate; launch/wait
 * failures, nonzero exit status, or signal termination throw. Foreground-terminal
 * behavior follows m03gagbhsvr0m5w15urj0o291m_process::create_and_wait_foreground_checked().
 */
void create_and_wait_checked(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t module, std::string_view target, int argc, const char* const* argv);

/**
 * @brief Builds a module's default CLI, invokes it, and waits for it to finish.
 *
 * @param module The module whose default CLI should be invoked.
 * @param args Literal additional arguments, excluding the executable name.
 *
 * Returns normally only after status 0. Build failures propagate; launch/wait
 * failures, nonzero exit status, or signal termination throw. Foreground-terminal
 * behavior follows m03gagbhsvr0m5w15urj0o291m_process::create_and_wait_foreground_checked().
 */
void create_and_wait_checked(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t module, std::vector<std::string> args);

/**
 * @brief Builds a module's selected binary target, invokes it, and waits for it to finish.
 *
 * @param module The module whose target should be invoked.
 * @param target The binary target to invoke.
 * @param args Literal additional arguments, excluding the executable name.
 *
 * Returns normally only after status 0. Build failures propagate; launch/wait
 * failures, nonzero exit status, or signal termination throw. Foreground-terminal
 * behavior follows m03gagbhsvr0m5w15urj0o291m_process::create_and_wait_foreground_checked().
 */
void create_and_wait_checked(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t module, std::string_view target, std::vector<std::string> args);

} // namespace m03gagbhst621faiop1rztfkqp_builder_cli

#endif // M03GAGBHST621FAIOP1RZTFKQP_BUILDER_CLI_BUILDER_CLI_H
