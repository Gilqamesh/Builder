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
 * Runs command as the terminal foreground job and throws unless it exits with status 0.
 */
void create_and_wait_foreground_checked(const command_t& command);

/**
 * Replaces the current process with command.
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
