#ifndef M03GAGBHSVR0M5W15URJ0O291M_PROCESS_PROCESS_H
# define M03GAGBHSVR0M5W15URJ0O291M_PROCESS_PROCESS_H

# include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>

# include <cstddef>
# include <format>
# include <optional>
# include <string>
# include <vector>

namespace m03gagbhsvr0m5w15urj0o291m_process {

/**
 * @brief Owns one environment binding to add or replace when executing a command.
 *
 * Construction throws std::invalid_argument for an empty name, '=' in the name,
 * or a null character in either string. Empty values are accepted.
 */
class environment_variable_t {
public:
    environment_variable_t(std::string name, std::string value);

    /** @brief Borrows the stored variable name for this object's lifetime, until assignment or move. */
    const std::string& name() const;
    /** @brief Borrows the stored variable value for this object's lifetime, until assignment or move. */
    const std::string& value() const;

private:
    std::string m_name;
    std::string m_value;
};

/**
 * @brief Owns literal command arguments, an optional working directory, and environment additions.
 *
 * args[0] is the executable path and is passed as argv[0]; PATH is not searched.
 * Relative executable paths resolve in working_dir, or the inherited directory
 * when working_dir is absent. Arguments are passed individually without shell
 * parsing, quoting, wildcard expansion, or variable substitution.
 *
 * Execution inherits the environment and applies additions in vector order,
 * replacing existing bindings; the last duplicate name wins. Construction only
 * stores these inputs: exec() validates arguments and applies the directory and
 * environment. See exec() for failures and side effects.
 * Accessor results borrow this command's storage; do not retain them across
 * destruction, assignment, or moving from the command.
 *
 * @code{.cpp}
 * #include <m03gagbhsvr0m5w15urj0o291m_process/process.h>
 * #include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
 *
 * int main() {
 *     namespace process = m03gagbhsvr0m5w15urj0o291m_process;
 *     namespace filesystem = m03gagbhsnusi43zogoacgj2ez_filesystem;
 *     const process::command_t command(
 *         {"/bin/echo", "hello world", "$HOME", "*"},
 *         filesystem::path_t("/tmp"),
 *         {process::environment_variable_t("DOC_LABEL", "demo run")}
 *     );
 *     process::create_and_wait_checked(command); // Prints: hello world $HOME *
 * }
 * @endcode
 */
class command_t {
public:
    command_t(std::vector<std::string> args, std::optional<m03gagbhsnusi43zogoacgj2ez_filesystem::path_t> working_dir = std::nullopt, std::vector<environment_variable_t> environment_variables = {});

    /** @brief Borrows the stored arguments, including the executable at index zero. */
    const std::vector<std::string>& args() const;
    /** @brief Borrows the directory selection; an absent value inherits the current directory. */
    const std::optional<m03gagbhsnusi43zogoacgj2ez_filesystem::path_t>& working_dir() const;
    /** @brief Borrows the ordered environment additions. */
    const std::vector<environment_variable_t>& environment_variables() const;

private:
    std::vector<std::string> m_args;
    std::optional<m03gagbhsnusi43zogoacgj2ez_filesystem::path_t> m_working_dir;
    std::vector<environment_variable_t> m_environment_variables;
};

/**
 * @brief Forks and executes a command, waits for it, and returns its exit status or negated signal number.
 *
 * A normal exit returns 0 through 255; only 0 denotes success. Child-side setup
 * or exec() failure is reported on stderr and becomes exit code 127, which is
 * indistinguishable from a program explicitly exiting with 127. Parent-side
 * setup, fork, and wait failures throw std::runtime_error.
 *
 * SIGINT, SIGTERM, and SIGHUP received by the parent are forwarded to the child;
 * after the wait, a received signal raises
 * m03gagbhsyhlx2pk5sdabbr1sx_signal_handler::termination_request_t.
 * @ref m03gagbhsyhlx2pk5sdabbr1sx_signal_handler::scoped_child_termination_guard_t "The child termination guard"
 * owns the signal policy and process-wide guard restrictions; overlapping child
 * guards are rejected.
 * The command is borrowed only for this call; directory and environment changes
 * occur in the child. Standard streams are inherited.
 */
int create_and_wait(const command_t& command);

/**
 * @brief Runs a command synchronously and throws unless it exits with status zero.
 *
 * Uses create_and_wait(), propagating its exceptions and throwing
 * std::runtime_error for a nonzero exit code or termination by signal.
 */
void create_and_wait_checked(const command_t& command);

/**
 * @brief Runs a command with foreground terminal handoff and throws unless it exits with status zero.
 *
 * When stdin is not a terminal, uses checked non-interactive execution semantics
 * as described by create_and_wait_checked(), without terminal handoff.
 * With a terminal, the child runs in its own process group and receives foreground
 * ownership; forwarded termination signals target that group.
 *
 * The previous foreground process group and saved terminal attributes are restored
 * after waiting, with best-effort restoration during exception unwinding. A stopped
 * foreground child is not a successful completion: after restoring the terminal,
 * its group is sent SIGKILL (falling back to the child), the child is reaped, and
 * std::runtime_error reports the stop. Terminal setup/restoration failures also
 * throw std::runtime_error; restoration cannot be guaranteed if the terminal
 * operations themselves fail. The signal policy and restrictions of
 * create_and_wait() also apply.
 */
void create_and_wait_foreground_checked(const command_t& command);

/**
 * @brief Applies a command's environment and directory, then replaces the current process image.
 *
 * Does not return on success or run C++ destructors. Throws std::runtime_error
 * for an empty argument vector or failed setenv(), chdir(), or execv(); embedded
 * null characters in arguments or the working directory throw std::invalid_argument.
 * Input validation precedes environment/directory changes, but later failures do
 * not roll back changes already applied to the current process. The executable
 * is a path, with no PATH search; the command is written to stderr before execv().
 */
[[noreturn]] void exec(const command_t& command);

} // namespace m03gagbhsvr0m5w15urj0o291m_process

namespace std {

template <>
struct formatter<m03gagbhsvr0m5w15urj0o291m_process::environment_variable_t>;

template <>
struct formatter<m03gagbhsvr0m5w15urj0o291m_process::command_t>;

} // namespace std

namespace std {

template <>
struct formatter<m03gagbhsvr0m5w15urj0o291m_process::environment_variable_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();

        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid environment_variable_t format specifier");
        }

        return it;
    }

    auto format(const m03gagbhsvr0m5w15urj0o291m_process::environment_variable_t& environment_variable, auto& ctx) const {
        auto out = ctx.out();

        out = std::format_to(out, "{{ name: {}, value: {} }}", environment_variable.name(), environment_variable.value());

        return out;
    }
};

template <>
struct formatter<m03gagbhsvr0m5w15urj0o291m_process::command_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();

        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid command_t format specifier");
        }

        return it;
    }

    auto format(const m03gagbhsvr0m5w15urj0o291m_process::command_t& command, auto& ctx) const {
        auto out = ctx.out();

        out = std::format_to(out, "{{ args: [");
        for (std::size_t i = 0; i < command.args().size(); ++i) {
            if (0 < i) {
                out = std::format_to(out, ", ");
            }
            out = std::format_to(out, "{}", command.args()[i]);
        }
        out = std::format_to(out, "], working_dir: ");

        if (command.working_dir()) {
            out = std::format_to(out, "{}", *command.working_dir());
        } else {
            out = std::format_to(out, "null");
        }

        out = std::format_to(out, ", environment_variables: [");
        for (std::size_t i = 0; i < command.environment_variables().size(); ++i) {
            if (0 < i) {
                out = std::format_to(out, ", ");
            }
            out = std::format_to(out, "{}", command.environment_variables()[i]);
        }
        out = std::format_to(out, "] }}");

        return out;
    }
};

} // namespace std

#endif // M03GAGBHSVR0M5W15URJ0O291M_PROCESS_PROCESS_H
