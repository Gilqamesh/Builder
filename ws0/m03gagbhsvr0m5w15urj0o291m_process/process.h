#ifndef M03GAGBHSVR0M5W15URJ0O291M_PROCESS_PROCESS_H
# define M03GAGBHSVR0M5W15URJ0O291M_PROCESS_PROCESS_H

# include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>

# include <optional>
# include <string>
# include <variant>
# include <vector>

namespace m03gagbhsvr0m5w15urj0o291m_process {

/**
 * Environment variable name and value for a child process.
 */
class environment_variable_t {
public:
    environment_variable_t(std::string name, std::string value);

    const std::string& name() const;
    const std::string& value() const;

private:
    std::string m_name;
    std::string m_value;
};

/**
 * Process command line, optional working directory, and environment additions.
 */
class command_t {
public:
    command_t(std::vector<std::string> args, std::optional<m03gagbhsnusi43zogoacgj2ez_filesystem::path_t> working_dir = std::nullopt, std::vector<environment_variable_t> environment_variables = {});

    const std::vector<std::string>& args() const;
    const std::optional<m03gagbhsnusi43zogoacgj2ez_filesystem::path_t>& working_dir() const;
    const std::vector<environment_variable_t>& environment_variables() const;

private:
    std::vector<std::string> m_args;
    std::optional<m03gagbhsnusi43zogoacgj2ez_filesystem::path_t> m_working_dir;
    std::vector<environment_variable_t> m_environment_variables;
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
