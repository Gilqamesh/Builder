#include "process.h"

#include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
#include <m03gagbhsyhlx2pk5sdabbr1sx_signal_handler/signal_handler.h>

#include <iostream>
#include <cerrno>
#include <cstdlib>
#include <format>
#include <stdexcept>
#include <type_traits>

#include <unistd.h>
#include <sys/wait.h>
#include <cstring>

namespace m03gagbhsvr0m5w15urj0o291m_process {

environment_variable_t::environment_variable_t(std::string name, std::string value):
    m_name(std::move(name)),
    m_value(std::move(value))
{
    if (m_name.empty()) {
        throw std::invalid_argument("m03gagbhsvr0m5w15urj0o291m_process::environment_variable_t: name must not be empty");
    }

    if (m_name.find('=') != std::string::npos) {
        throw std::invalid_argument(std::format("m03gagbhsvr0m5w15urj0o291m_process::environment_variable_t: name '{}' must not contain '='", m_name));
    }
}

const std::string& environment_variable_t::name() const {
    return m_name;
}

const std::string& environment_variable_t::value() const {
    return m_value;
}

command_t::command_t(
    std::vector<std::string> args,
    std::optional<m03gagbhsnusi43zogoacgj2ez_filesystem::path_t> working_dir,
    std::vector<environment_variable_t> environment_variables
):
    m_args(std::move(args)),
    m_working_dir(std::move(working_dir)),
    m_environment_variables(std::move(environment_variables))
{
}

const std::vector<std::string>& command_t::args() const {
    return m_args;
}

const std::optional<m03gagbhsnusi43zogoacgj2ez_filesystem::path_t>& command_t::working_dir() const {
    return m_working_dir;
}

const std::vector<environment_variable_t>& command_t::environment_variables() const {
    return m_environment_variables;
}

int create_and_wait(const command_t& command) {
    int status = 0;
    {
        m03gagbhsyhlx2pk5sdabbr1sx_signal_handler::scoped_child_termination_guard_t termination_guard([&]() {
            try {
                exec(command);
            } catch (const std::exception& e) {
                dprintf(STDERR_FILENO, "m03gagbhsvr0m5w15urj0o291m_process::create_and_wait: child command failed: %s\n", e.what());
                _exit(127);
            }
        });

        while (waitpid(termination_guard.pid(), &status, 0) == -1) {
            if (errno == EINTR) {
                continue ;
            }

            throw std::runtime_error(std::format("m03gagbhsvr0m5w15urj0o291m_process::create_and_wait: waitpid failed: {}", std::strerror(errno)));
        }
    }

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        const int return_value = -WTERMSIG(status);
        if (0 <= return_value) {
            throw std::runtime_error(std::format("m03gagbhsvr0m5w15urj0o291m_process::create_and_wait: unreachable state reached after waitpid, WIFSIGNALED but non-negative return value: {}", return_value));
        }
        return return_value;
    } else {
        throw std::runtime_error(std::format("m03gagbhsvr0m5w15urj0o291m_process::create_and_wait: unreachable state reached after waitpid, status: {}", status));
    }
}

void create_and_wait_checked(const command_t& command) {
    const int process_result = create_and_wait(command);

    if (0 < process_result) {
        throw std::runtime_error(std::format("m03gagbhsvr0m5w15urj0o291m_process::create_and_wait_checked: process exited with non-zero exit code: {}", process_result));
    } else if (process_result < 0) {
        throw std::runtime_error(std::format("m03gagbhsvr0m5w15urj0o291m_process::create_and_wait_checked: process terminated by signal: {}", -process_result));
    }
}

[[noreturn]] void exec(const command_t& command) {
    for (const auto& environment_variable : command.environment_variables()) {
        if (setenv(environment_variable.name().c_str(), environment_variable.value().c_str(), 1) == -1) {
            throw std::runtime_error(std::format("m03gagbhsvr0m5w15urj0o291m_process::exec: failed to set environment binding '{}': {}", environment_variable.name(), std::strerror(errno)));
        }
    }

    const auto& working_dir = command.working_dir();
    if (working_dir && chdir(working_dir->c_str()) == -1) {
        throw std::runtime_error(std::format("m03gagbhsvr0m5w15urj0o291m_process::exec: chdir failed for '{}': {}", *working_dir, std::strerror(errno)));
    }

    const auto& args = command.args();
    if (args.empty()) {
        throw std::runtime_error("m03gagbhsvr0m5w15urj0o291m_process::exec: command args must not be empty");
    }

    std::vector<char*> cargs;
    for (const auto& arg : args) {
        cargs.push_back(const_cast<char*>(arg.c_str()));
    }
    cargs.push_back(nullptr);

    if (execv(cargs[0], cargs.data()) == -1) {
        throw std::runtime_error(std::format("m03gagbhsvr0m5w15urj0o291m_process::exec: execv failed: {}", std::strerror(errno)));
    }
    throw std::runtime_error("m03gagbhsvr0m5w15urj0o291m_process::exec: unreachable state reached after execv");
}

} // namespace m03gagbhsvr0m5w15urj0o291m_process
