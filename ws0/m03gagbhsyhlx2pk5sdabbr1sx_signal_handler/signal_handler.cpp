#include "signal_handler.h"

#include <array>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <exception>
#include <format>
#include <unistd.h>

namespace m03gagbhsyhlx2pk5sdabbr1sx_signal_handler {

static constexpr std::array<int, 3> TERMINATION_SIGNALS = {
    SIGINT,
    SIGTERM,
    SIGHUP
};

static volatile sig_atomic_t g_termination_signal = 0;
static volatile sig_atomic_t g_child_pid = 0;
static volatile sig_atomic_t g_child_signal = 0;
static volatile sig_atomic_t g_child_guard_active = 0;

// Signal handlers can only do minimal async-signal-safe work. These handlers
// record process-global state for the owning guard to observe later; they are
// not a general thread synchronization mechanism.
static void request_termination(int signal_number) {
    if (g_termination_signal == 0) {
        g_termination_signal = signal_number;
        return ;
    }

    _exit(128 + signal_number);
}

static void forward_termination_to_child(int signal_number) {
    if (g_child_signal != 0) {
        _exit(128 + signal_number);
    }

    g_child_signal = signal_number;
    if (g_child_pid > 0) {
        kill(g_child_pid, signal_number);
    }
}

static void install_handler(int signal_number, struct sigaction& previous_action) {
    struct sigaction action {};
    action.sa_handler = request_termination;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    if (sigaction(signal_number, &action, &previous_action) == -1) {
        throw std::runtime_error(std::format(
            "m03gagbhsyhlx2pk5sdabbr1sx_signal_handler::scoped_termination_guard_t: failed to install handler for signal {}: {}",
            signal_number,
            std::strerror(errno)
        ));
    }
}

static void restore_handler(int signal_number, const struct sigaction& previous_action) {
    if (sigaction(signal_number, &previous_action, nullptr) == -1) {
        _exit(128 + signal_number);
    }
}

static void block_termination_signals(sigset_t& previous_mask) {
    sigset_t blocked_signals;
    sigemptyset(&blocked_signals);

    for (const auto signal_number : TERMINATION_SIGNALS) {
        sigaddset(&blocked_signals, signal_number);
    }

    if (sigprocmask(SIG_BLOCK, &blocked_signals, &previous_mask) == -1) {
        throw std::runtime_error(std::format(
            "m03gagbhsyhlx2pk5sdabbr1sx_signal_handler::scoped_child_termination_guard_t: failed to block termination signals: {}",
            std::strerror(errno)
        ));
    }
}

static void block_termination_signals_or_exit(sigset_t& previous_mask) {
    sigset_t blocked_signals;
    sigemptyset(&blocked_signals);

    for (const auto signal_number : TERMINATION_SIGNALS) {
        sigaddset(&blocked_signals, signal_number);
    }

    if (sigprocmask(SIG_BLOCK, &blocked_signals, &previous_mask) == -1) {
        _exit(128 + SIGTERM);
    }
}

static void restore_signal_mask(const sigset_t& previous_mask) {
    if (sigprocmask(SIG_SETMASK, &previous_mask, nullptr) == -1) {
        throw std::runtime_error(std::format(
            "m03gagbhsyhlx2pk5sdabbr1sx_signal_handler::scoped_child_termination_guard_t: failed to restore signal mask: {}",
            std::strerror(errno)
        ));
    }
}

static void restore_signal_mask_or_exit(const sigset_t& previous_mask) {
    if (sigprocmask(SIG_SETMASK, &previous_mask, nullptr) == -1) {
        _exit(128 + SIGTERM);
    }
}

termination_request_t::termination_request_t(int signal_number):
    std::runtime_error(std::format("termination requested by signal {}", signal_number)),
    m_signal_number(signal_number)
{
}

int termination_request_t::signal_number() const {
    return m_signal_number;
}

scoped_termination_guard_t::scoped_termination_guard_t() {
    if (g_termination_signal != 0) {
        throw termination_request_t(g_termination_signal);
    }

    try {
        for (std::size_t i = 0; i < TERMINATION_SIGNALS.size(); ++i) {
            install_handler(TERMINATION_SIGNALS[i], m_previous_actions[i]);
            m_active[i] = true;
        }
    } catch (...) {
        for (std::size_t i = TERMINATION_SIGNALS.size(); 0 < i; --i) {
            if (m_active[i - 1]) {
                restore_handler(TERMINATION_SIGNALS[i - 1], m_previous_actions[i - 1]);
            }
        }
        throw ;
    }
}

scoped_termination_guard_t::~scoped_termination_guard_t() noexcept(false) {
    for (std::size_t i = TERMINATION_SIGNALS.size(); 0 < i; --i) {
        if (m_active[i - 1]) {
            restore_handler(TERMINATION_SIGNALS[i - 1], m_previous_actions[i - 1]);
            m_active[i - 1] = false;
        }
    }

    if (std::uncaught_exceptions() == 0 && g_termination_signal != 0) {
        throw termination_request_t(g_termination_signal);
    }
}

void scoped_child_termination_guard_t::prepare() {
    block_termination_signals(m_previous_mask);
    m_mask_active = true;

    try {
        if (g_child_guard_active != 0) {
            throw std::runtime_error("m03gagbhsyhlx2pk5sdabbr1sx_signal_handler::scoped_child_termination_guard_t: nested child termination guards are not supported");
        }

        g_child_guard_active = 1;
        g_child_pid = 0;
        g_child_signal = 0;
        m_registered = true;

        for (std::size_t i = 0; i < TERMINATION_SIGNALS.size(); ++i) {
            struct sigaction action {};
            action.sa_handler = forward_termination_to_child;
            sigemptyset(&action.sa_mask);
            action.sa_flags = 0;

            if (sigaction(TERMINATION_SIGNALS[i], &action, &m_previous_actions[i]) == -1) {
                throw std::runtime_error(std::format(
                    "m03gagbhsyhlx2pk5sdabbr1sx_signal_handler::scoped_child_termination_guard_t: failed to install handler for signal {}: {}",
                    TERMINATION_SIGNALS[i],
                    std::strerror(errno)
                ));
            }
            m_handler_active[i] = true;
        }
    } catch (...) {
        cleanup_or_exit();
        throw ;
    }
}

pid_t scoped_child_termination_guard_t::fork_child() {
    const auto child_pid = fork();
    if (child_pid == -1) {
        const auto error_number = errno;
        cleanup_or_exit();
        throw std::runtime_error(std::format(
            "m03gagbhsyhlx2pk5sdabbr1sx_signal_handler::scoped_child_termination_guard_t: fork failed: {}",
            std::strerror(error_number)
        ));
    }

    return child_pid;
}

void scoped_child_termination_guard_t::enter_child() {
    cleanup_or_exit();
}

void scoped_child_termination_guard_t::enter_parent(pid_t pid) {
    m_pid = pid;
    g_child_pid = pid;

    try {
        restore_signal_mask(m_previous_mask);
        m_mask_active = false;
    } catch (...) {
        cleanup_or_exit();
        throw ;
    }
}

void scoped_child_termination_guard_t::cleanup_or_exit() noexcept {
    for (std::size_t i = TERMINATION_SIGNALS.size(); 0 < i; --i) {
        if (m_handler_active[i - 1]) {
            restore_handler(TERMINATION_SIGNALS[i - 1], m_previous_actions[i - 1]);
            m_handler_active[i - 1] = false;
        }
    }

    if (m_registered) {
        g_child_pid = 0;
        g_child_signal = 0;
        g_child_guard_active = 0;
        m_registered = false;
    }

    if (m_mask_active) {
        restore_signal_mask_or_exit(m_previous_mask);
        m_mask_active = false;
    }
}

scoped_child_termination_guard_t::~scoped_child_termination_guard_t() noexcept(false) {
    if (m_registered && !m_mask_active) {
        block_termination_signals_or_exit(m_previous_mask);
        m_mask_active = true;
    }

    const int signal_number = g_child_signal;

    cleanup_or_exit();

    if (std::uncaught_exceptions() == 0 && signal_number != 0) {
        throw termination_request_t(signal_number);
    }
}

pid_t scoped_child_termination_guard_t::pid() const {
    return m_pid;
}

} // namespace m03gagbhsyhlx2pk5sdabbr1sx_signal_handler
