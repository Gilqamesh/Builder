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

static void apply_environment(const std::vector<environment_binding_t>& environment) {
    for (const auto& binding : environment) {
        if (binding.name.empty()) {
            throw std::runtime_error("m03gagbhsvr0m5w15urj0o291m_process::exec: environment binding name must not be empty");
        }
        if (binding.name.find('=') != std::string::npos) {
            throw std::runtime_error(std::format("m03gagbhsvr0m5w15urj0o291m_process::exec: environment binding name '{}' must not contain '='", binding.name));
        }
        if (setenv(binding.name.c_str(), binding.value.c_str(), 1) == -1) {
            throw std::runtime_error(std::format("m03gagbhsvr0m5w15urj0o291m_process::exec: failed to set environment binding '{}': {}", binding.name, std::strerror(errno)));
        }
    }
}

static void apply_working_dir(const std::optional<m03gagbhsnusi43zogoacgj2ez_filesystem::path_t>& working_dir) {
    if (working_dir && chdir(working_dir->c_str()) == -1) {
        throw std::runtime_error(std::format("m03gagbhsvr0m5w15urj0o291m_process::exec: chdir failed for '{}': {}", *working_dir, std::strerror(errno)));
    }
}

static std::vector<char*> cargs(const std::vector<process_arg_t>& args, std::string& pretty_print) {
    if (args.empty()) {
        throw std::runtime_error("m03gagbhsvr0m5w15urj0o291m_process::exec: command args must not be empty");
    }

    std::vector<char*> result;
    for (const auto& arg : args) {
        if (!pretty_print.empty()) {
            pretty_print += " ";
        }
        pretty_print += std::visit(
            [&](auto&& v) {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, std::string>) {
                    result.push_back(const_cast<char*>(v.c_str()));
                    return v;
                } else if constexpr (std::is_same_v<T, m03gagbhsnusi43zogoacgj2ez_filesystem::path_t>) {
                    result.push_back(const_cast<char*>(v.c_str()));
                    return m03gagbhsnusi43zogoacgj2ez_filesystem::pretty_path_t(v).string();
                } else {
                    static_assert(false, "non-exhaustive visitor!");
                }
            },
            arg
        );
    }
    result.push_back(nullptr);
    return result;
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
    apply_environment(command.environment);
    apply_working_dir(command.working_dir);

    std::string pretty_print;
    auto exec_args = cargs(command.args, pretty_print);

    std::cout << pretty_print << std::endl;
    if (execv(exec_args[0], exec_args.data()) == -1) {
        throw std::runtime_error(std::format("m03gagbhsvr0m5w15urj0o291m_process::exec: execv failed: {}", std::strerror(errno)));
    }
    throw std::runtime_error("m03gagbhsvr0m5w15urj0o291m_process::exec: unreachable state reached after execv");
}

} // namespace m03gagbhsvr0m5w15urj0o291m_process
