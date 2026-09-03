#include "foreground_job.h"

#include "process.h"

#include <m03gagbhsyhlx2pk5sdabbr1sx_signal_handler/signal_handler.h>

#include <cerrno>
#include <csignal>
#include <cstring>
#include <exception>
#include <format>
#include <stdexcept>

#include <sys/wait.h>
#include <unistd.h>

namespace m03gagbhsvr0m5w15urj0o291m_process {

static void set_foreground_process_group(int terminal_fd, pid_t process_group, const char* operation) {
    sigset_t blocked_signals;
    sigemptyset(&blocked_signals);
    sigaddset(&blocked_signals, SIGTTOU);

    sigset_t previous_mask;
    if (sigprocmask(SIG_BLOCK, &blocked_signals, &previous_mask) == -1) {
        throw std::runtime_error(std::format("m03gagbhsvr0m5w15urj0o291m_process::foreground_job_t: failed to block SIGTTOU: {}", std::strerror(errno)));
    }

    const int result = tcsetpgrp(terminal_fd, process_group);
    const int error_number = errno;
    if (sigprocmask(SIG_SETMASK, &previous_mask, nullptr) == -1) {
        throw std::runtime_error(std::format("m03gagbhsvr0m5w15urj0o291m_process::foreground_job_t: failed to restore signal mask: {}", std::strerror(errno)));
    }

    if (result == -1) {
        throw std::runtime_error(std::format("m03gagbhsvr0m5w15urj0o291m_process::foreground_job_t: {} failed for process group {}: {}", operation, process_group, std::strerror(error_number)));
    }
}

static bool set_foreground_process_group_noexcept(int terminal_fd, pid_t process_group) noexcept {
    sigset_t blocked_signals;
    sigemptyset(&blocked_signals);
    sigaddset(&blocked_signals, SIGTTOU);

    sigset_t previous_mask;
    if (sigprocmask(SIG_BLOCK, &blocked_signals, &previous_mask) == -1) {
        return false;
    }

    const bool success = tcsetpgrp(terminal_fd, process_group) != -1;
    const bool mask_restored = sigprocmask(SIG_SETMASK, &previous_mask, nullptr) != -1;
    return mask_restored && success;
}

static void close_descriptor_noexcept(int& descriptor) noexcept {
    if (descriptor == -1) {
        return ;
    }
    close(descriptor);
    descriptor = -1;
}

static void close_descriptor(int& descriptor, const char* operation) {
    if (descriptor == -1) {
        return ;
    }
    if (close(descriptor) == -1) {
        const int error_number = errno;
        descriptor = -1;
        throw std::runtime_error(std::format("m03gagbhsvr0m5w15urj0o291m_process::foreground_job_t: {} failed: {}", operation, std::strerror(error_number)));
    }
    descriptor = -1;
}

static void write_gate_byte(int descriptor) {
    const char byte = 0;
    while (true) {
        const ssize_t count = write(descriptor, &byte, 1);
        if (count == 1) {
            return ;
        }
        if (count == -1 && errno == EINTR) {
            continue ;
        }
        throw std::runtime_error(std::format("m03gagbhsvr0m5w15urj0o291m_process::foreground_job_t: foreground handoff write failed: {}", count == -1 ? std::strerror(errno) : "short write"));
    }
}

static void read_gate_byte(int descriptor) {
    char byte = 0;
    while (true) {
        const ssize_t count = read(descriptor, &byte, 1);
        if (count == 1) {
            return ;
        }
        if (count == -1 && errno == EINTR) {
            continue ;
        }
        throw std::runtime_error(std::format("m03gagbhsvr0m5w15urj0o291m_process::foreground_job_t: foreground handoff read failed: {}", count == -1 ? std::strerror(errno) : "parent closed handoff gate"));
    }
}

static void wait_after_forced_kill(pid_t child_pid) noexcept {
    int ignored_status = 0;
    while (waitpid(child_pid, &ignored_status, 0) == -1) {
        if (errno == EINTR) {
            continue ;
        }
        return ;
    }
}

int foreground_job_t::create_and_wait(const command_t& command) {
    int status = 0;
    foreground_job_t foreground_job;
    {
        m03gagbhsyhlx2pk5sdabbr1sx_signal_handler::scoped_child_termination_guard_t termination_guard(foreground_job.child_signal_target());
        const auto child_pid = fork();
        if (child_pid == -1) {
            throw std::runtime_error(std::format("m03gagbhsvr0m5w15urj0o291m_process::foreground_job_t::create_and_wait: fork failed: {}", std::strerror(errno)));
        }

        if (child_pid == 0) {
            try {
                foreground_job.prepare_child();
                termination_guard.enter_child();
                exec(command);
            } catch (const std::exception& e) {
                dprintf(STDERR_FILENO, "m03gagbhsvr0m5w15urj0o291m_process::foreground_job_t::create_and_wait: child command failed: %s\n", e.what());
                _exit(127);
            }
        }

        try {
            foreground_job.prepare_parent(child_pid);
            termination_guard.enter_parent(child_pid);
        } catch (...) {
            foreground_job.kill_child();
            kill(child_pid, SIGKILL);
            wait_after_forced_kill(child_pid);
            throw ;
        }

        while (waitpid(child_pid, &status, foreground_job.wait_options()) == -1) {
            if (errno == EINTR) {
                continue ;
            }

            throw std::runtime_error(std::format("m03gagbhsvr0m5w15urj0o291m_process::foreground_job_t::create_and_wait: waitpid failed: {}", std::strerror(errno)));
        }

        foreground_job.restore();

        if (WIFSTOPPED(status)) {
            foreground_job.kill_child();
            wait_after_forced_kill(child_pid);
            throw std::runtime_error(std::format(
                "m03gagbhsvr0m5w15urj0o291m_process::foreground_job_t::create_and_wait: process stopped by signal: {}",
                WSTOPSIG(status)
            ));
        }
    }

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        const int return_value = -WTERMSIG(status);
        if (0 <= return_value) {
            throw std::runtime_error(std::format("m03gagbhsvr0m5w15urj0o291m_process::foreground_job_t::create_and_wait: unreachable state reached after waitpid, WIFSIGNALED but non-negative return value: {}", return_value));
        }
        return return_value;
    } else {
        throw std::runtime_error(std::format("m03gagbhsvr0m5w15urj0o291m_process::foreground_job_t::create_and_wait: unreachable state reached after waitpid, status: {}", status));
    }
}

foreground_job_t::foreground_job_t():
    m_active(isatty(STDIN_FILENO) == 1),
    m_child_pid(-1),
    m_previous_foreground_process_group(-1),
    m_saved_terminal_attributes(),
    m_terminal_attributes_saved(false),
    m_foreground_transferred(false),
    m_child_gate_read_descriptor(-1),
    m_child_gate_write_descriptor(-1)
{
    if (!m_active) {
        return ;
    }

    m_previous_foreground_process_group = tcgetpgrp(STDIN_FILENO);
    if (m_previous_foreground_process_group == -1) {
        if (errno == ENOTTY) {
            m_active = false;
            return ;
        }
        throw std::runtime_error(std::format("m03gagbhsvr0m5w15urj0o291m_process::foreground_job_t: tcgetpgrp failed: {}", std::strerror(errno)));
    }

    if (tcgetattr(STDIN_FILENO, &m_saved_terminal_attributes) == -1) {
        if (errno == ENOTTY) {
            m_active = false;
            return ;
        }
        throw std::runtime_error(std::format("m03gagbhsvr0m5w15urj0o291m_process::foreground_job_t: tcgetattr failed: {}", std::strerror(errno)));
    }
    m_terminal_attributes_saved = true;

    int descriptors[2] = {-1, -1};
    if (pipe(descriptors) == -1) {
        throw std::runtime_error(std::format("m03gagbhsvr0m5w15urj0o291m_process::foreground_job_t: pipe failed for foreground handoff: {}", std::strerror(errno)));
    }
    m_child_gate_read_descriptor = descriptors[0];
    m_child_gate_write_descriptor = descriptors[1];
}

foreground_job_t::~foreground_job_t() {
    restore_noexcept();
}

bool foreground_job_t::active() const {
    return m_active;
}

pid_t foreground_job_t::child_pid() const {
    return m_child_pid;
}

pid_t foreground_job_t::previous_foreground_process_group() const {
    return m_previous_foreground_process_group;
}

bool foreground_job_t::terminal_attributes_saved() const {
    return m_terminal_attributes_saved;
}

bool foreground_job_t::foreground_transferred() const {
    return m_foreground_transferred;
}

int foreground_job_t::child_gate_read_descriptor() const {
    return m_child_gate_read_descriptor;
}

int foreground_job_t::child_gate_write_descriptor() const {
    return m_child_gate_write_descriptor;
}

int foreground_job_t::wait_options() const {
    return m_active ? WUNTRACED : 0;
}

m03gagbhsyhlx2pk5sdabbr1sx_signal_handler::child_signal_target_t foreground_job_t::child_signal_target() const {
    if (m_active) {
        return m03gagbhsyhlx2pk5sdabbr1sx_signal_handler::child_signal_target_t::process_group;
    }
    return m03gagbhsyhlx2pk5sdabbr1sx_signal_handler::child_signal_target_t::process;
}

void foreground_job_t::prepare_child() {
    if (!m_active) {
        return ;
    }
    if (setpgid(0, 0) == -1) {
        throw std::runtime_error(std::format("m03gagbhsvr0m5w15urj0o291m_process::foreground_job_t: child setpgid failed: {}", std::strerror(errno)));
    }
    close_descriptor(m_child_gate_write_descriptor, "close child foreground handoff write descriptor");
    read_gate_byte(m_child_gate_read_descriptor);
    close_descriptor(m_child_gate_read_descriptor, "close child foreground handoff read descriptor");
}

void foreground_job_t::prepare_parent(pid_t child_pid) {
    if (!m_active) {
        return ;
    }
    m_child_pid = child_pid;
    close_descriptor(m_child_gate_read_descriptor, "close parent foreground handoff read descriptor");
    if (setpgid(child_pid, child_pid) == -1 && errno != EACCES) {
        if (errno == ESRCH) {
            close_descriptor(m_child_gate_write_descriptor, "close parent foreground handoff write descriptor");
            return ;
        }
        throw std::runtime_error(std::format("m03gagbhsvr0m5w15urj0o291m_process::foreground_job_t: parent setpgid failed for child {}: {}", child_pid, std::strerror(errno)));
    }
    set_foreground_process_group(STDIN_FILENO, child_pid, "tcsetpgrp to child");
    m_foreground_transferred = true;
    write_gate_byte(m_child_gate_write_descriptor);
    close_descriptor(m_child_gate_write_descriptor, "close parent foreground handoff write descriptor");
}

void foreground_job_t::restore() {
    if (!m_active) {
        return ;
    }
    close_descriptor(m_child_gate_read_descriptor, "close foreground handoff read descriptor");
    close_descriptor(m_child_gate_write_descriptor, "close foreground handoff write descriptor");
    if (m_foreground_transferred) {
        set_foreground_process_group(STDIN_FILENO, m_previous_foreground_process_group, "tcsetpgrp to parent");
        m_foreground_transferred = false;
    }
    if (m_terminal_attributes_saved) {
        if (tcsetattr(STDIN_FILENO, TCSADRAIN, &m_saved_terminal_attributes) == -1) {
            throw std::runtime_error(std::format("m03gagbhsvr0m5w15urj0o291m_process::foreground_job_t: tcsetattr failed while restoring terminal: {}", std::strerror(errno)));
        }
        m_terminal_attributes_saved = false;
    }
    m_active = false;
}

void foreground_job_t::restore_noexcept() noexcept {
    if (!m_active) {
        return ;
    }
    close_descriptor_noexcept(m_child_gate_read_descriptor);
    close_descriptor_noexcept(m_child_gate_write_descriptor);
    if (m_foreground_transferred && set_foreground_process_group_noexcept(STDIN_FILENO, m_previous_foreground_process_group)) {
        m_foreground_transferred = false;
    }
    if (m_terminal_attributes_saved && tcsetattr(STDIN_FILENO, TCSADRAIN, &m_saved_terminal_attributes) != -1) {
        m_terminal_attributes_saved = false;
    }
    if (!m_foreground_transferred && !m_terminal_attributes_saved) {
        m_active = false;
    }
}

void foreground_job_t::kill_child() const noexcept {
    if (m_child_pid == -1) {
        return ;
    }
    if (kill(-m_child_pid, SIGKILL) != -1) {
        return ;
    }
    kill(m_child_pid, SIGKILL);
}

} // namespace m03gagbhsvr0m5w15urj0o291m_process
