#ifndef M03GAGBHSVR0M5W15URJ0O291M_PROCESS_PROCESS_H
# define M03GAGBHSVR0M5W15URJ0O291M_PROCESS_PROCESS_H

# include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>

# include <optional>
# include <string>
# include <variant>
# include <vector>

namespace m03gagbhsvr0m5w15urj0o291m_process {

/**
 * Command argument passed as either a string or filesystem path.
 */
using process_arg_t = std::variant<std::string, m03gagbhsnusi43zogoacgj2ez_filesystem::path_t>;

/**
 * Environment variable value for a child process.
 */
struct environment_binding_t {
    std::string name;
    std::string value;
};

/**
 * Process command line, optional working directory, and environment additions.
 */
struct command_t {
    std::vector<process_arg_t> args;
    std::optional<m03gagbhsnusi43zogoacgj2ez_filesystem::path_t> working_dir;
    std::vector<environment_binding_t> environment;
};

/**
 * Runs command and waits for it to finish.
 *
 * Returns a non-negative exit code on success, or the negated signal number when the process terminates by signal.
 */
int create_and_wait(const command_t& command);

/**
 * Runs command and throws unless it exits with status 0.
 */
void create_and_wait_checked(const command_t& command);

/**
 * Replaces the current process with command.
 */
[[noreturn]] void exec(const command_t& command);

} // namespace m03gagbhsvr0m5w15urj0o291m_process

#endif // M03GAGBHSVR0M5W15URJ0O291M_PROCESS_PROCESS_H
