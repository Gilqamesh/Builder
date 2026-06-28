#ifndef M03GAGBHSYHLX2PK5SDABBR1SX_SIGNAL_HANDLER_SIGNAL_HANDLER_H
# define M03GAGBHSYHLX2PK5SDABBR1SX_SIGNAL_HANDLER_SIGNAL_HANDLER_H

# include <array>
# include <signal.h>
# include <stdexcept>
# include <sys/types.h>
# include <unistd.h>
# include <utility>

namespace m03gagbhsyhlx2pk5sdabbr1sx_signal_handler {

/**
 * Exception raised for SIGINT, SIGTERM, or SIGHUP.
 */
class termination_request_t : public std::runtime_error {
public:
    explicit termination_request_t(int signal_number);

    /** Returns the POSIX signal that requested shutdown. */
    int signal_number() const;

private:
    int m_signal_number;
};

/**
 * Records SIGINT, SIGTERM, and SIGHUP for this scope.
 *
 * The destructor restores the previous handlers and throws termination_request_t if a signal was received.
 */
class scoped_termination_guard_t {
public:
    scoped_termination_guard_t();
    ~scoped_termination_guard_t() noexcept(false);

    scoped_termination_guard_t(const scoped_termination_guard_t&) = delete;
    scoped_termination_guard_t& operator=(const scoped_termination_guard_t&) = delete;

private:
    std::array<struct sigaction, 3> m_previous_actions;
    std::array<bool, 3> m_active {};
};

/**
 * Forks child_fn and forwards SIGINT, SIGTERM, and SIGHUP to the child.
 *
 * In the parent, pid() returns the child pid. If child_fn returns, the child exits with 0; if child_fn throws, the child exits with 127.
 */
class scoped_child_termination_guard_t {
public:
    template <class child_fn_t>
    explicit scoped_child_termination_guard_t(child_fn_t&& child_fn);
    ~scoped_child_termination_guard_t() noexcept(false);

    scoped_child_termination_guard_t(const scoped_child_termination_guard_t&) = delete;
    scoped_child_termination_guard_t& operator=(const scoped_child_termination_guard_t&) = delete;

    /** Returns the forked child pid in the parent process. */
    pid_t pid() const;

private:
    void prepare();
    pid_t fork_child();
    void enter_child();
    void enter_parent(pid_t pid);
    void cleanup_or_exit() noexcept;

private:
    pid_t m_pid = -1;
    std::array<struct sigaction, 3> m_previous_actions {};
    std::array<bool, 3> m_handler_active {};
    sigset_t m_previous_mask {};
    bool m_mask_active = false;
    bool m_registered = false;
};

template <class child_fn_t>
scoped_child_termination_guard_t::scoped_child_termination_guard_t(child_fn_t&& child_fn) {
    prepare();

    const auto child_pid = fork_child();
    if (child_pid == 0) {
        try {
            enter_child();
            std::forward<child_fn_t>(child_fn)();
        } catch (...) {
            _exit(127);
        }

        _exit(0);
    }

    enter_parent(child_pid);
}

} // namespace m03gagbhsyhlx2pk5sdabbr1sx_signal_handler

#endif // M03GAGBHSYHLX2PK5SDABBR1SX_SIGNAL_HANDLER_SIGNAL_HANDLER_H
