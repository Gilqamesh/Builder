#include "process.h"

#include <iostream>
#include <format>

#include <unistd.h>
#include <sys/wait.h>
#include <cstring>

namespace process {

int create_and_wait(const std::vector<process_arg_t>& args) {
    const auto pid = fork();
    if (pid == -1) {
        throw std::runtime_error(std::format("process::create_and_wait: fork failed: {}", std::strerror(errno)));
    }

    if (pid == 0) {
        exec(args);
    }

    int status = 0;
    if (waitpid(pid, &status, 0) == -1) {
        throw std::runtime_error(std::format("process::create_and_wait: waitpid failed: {}", std::strerror(errno)));
    }

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        const int return_value = -WTERMSIG(status);
        if (0 <= return_value) {
            throw std::runtime_error(std::format("process::create_and_wait: unreachable state reached after waitpid, WIFSIGNALED but non-negative return value: {}", return_value));
        }
        return return_value;
    } else {
        throw std::runtime_error(std::format("process::create_and_wait: unreachable state reached after waitpid, status: {}", status));
    }
}

[[noreturn]] void exec(const std::vector<process_arg_t>& args) {
    std::vector<char*> cargs;

    std::string pretty_print;
    for (const auto& arg : args) {
        if (!pretty_print.empty()) {
            pretty_print += " ";
        }
        pretty_print += std::visit(
            [&](auto&& v) {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, std::string>) {
                    cargs.push_back(const_cast<char*>(v.c_str()));
                    return v;
                } else if constexpr (std::is_same_v<T, filesystem::path_t>) {
                    cargs.push_back(const_cast<char*>(v.c_str()));
                    return filesystem::pretty_path_t(v).string();
                } else {
                    static_assert(false, "non-exhaustive visitor!");
                }
            },
            arg
        );
    }
    cargs.push_back(nullptr);

    std::cout << pretty_print << std::endl;
    if (execv(cargs[0], cargs.data()) == -1) {
        throw std::runtime_error(std::format("process::exec: execv failed: {}", std::strerror(errno)));
    }
    throw std::runtime_error("process::exec: unreachable state reached after execv");
}

} // namespace process
