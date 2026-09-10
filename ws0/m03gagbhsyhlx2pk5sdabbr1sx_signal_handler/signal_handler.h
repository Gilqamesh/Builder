#ifndef M03GAGBHSYHLX2PK5SDABBR1SX_SIGNAL_HANDLER_SIGNAL_HANDLER_H
# define M03GAGBHSYHLX2PK5SDABBR1SX_SIGNAL_HANDLER_SIGNAL_HANDLER_H

# include <array>
# include <signal.h>
# include <stdexcept>
# include <sys/types.h>
# include <unistd.h>
# include <utility>

namespace m03gagbhsyhlx2pk5sdabbr1sx_signal_handler {

/** @brief Selects the child process or its process group as the termination-signal destination. */
enum class child_signal_target_t {
    process, ///< Sends to the registered positive child pid.
    process_group ///< Sends to the group whose id equals the registered child pid; the caller establishes that group.
};

/**
 * @brief Carries the POSIX signal number that requested shutdown at a guard boundary.
 */
class termination_request_t : public std::runtime_error {
public:
    explicit termination_request_t(int signal_number);

    /** @brief Returns the stored POSIX signal number without translating it to an exit status. */
    int signal_number() const;

private:
    int m_signal_number;
};

/**
 * @brief Defers the first SIGINT, SIGTERM, or SIGHUP to a termination exception at scope exit.
 *
 * The signal handler records the request; execution continues until the scope
 * exits. The destructor restores the previous handlers and throws
 * termination_request_t only if a request is recorded and no exception is
 * already unwinding. Catch outside the guarded scope. A second handled
 * termination signal exits immediately with status 128 + signal number.
 *
 * Recorded requests persist: constructing another guard after a request throws
 * termination_request_t immediately. Handler installation failures throw
 * std::runtime_error; failure to restore a handler exits the process.
 * Handlers and request state are process-wide. Do not manage guards or change
 * these signal handlers concurrently; this API does not coordinate signal
 * delivery among multiple threads.
 *
 * @code{.cpp}
 * #include <m03gagbhsyhlx2pk5sdabbr1sx_signal_handler/signal_handler.h>
 * #include <csignal>
 * #include <stdexcept>
 *
 * int main() {
 *     namespace signals = m03gagbhsyhlx2pk5sdabbr1sx_signal_handler;
 *     try {
 *         signals::scoped_termination_guard_t scoped_termination_guard;
 *         if (std::raise(SIGTERM) != 0) {
 *             throw std::runtime_error("failed to request termination");
 *         }
 *         // Work reaches here; normal scope exit then raises the request.
 *     } catch (const signals::termination_request_t& termination_request) {
 *         return termination_request.signal_number() == SIGTERM ? 0 : 1;
 *     }
 *     return 1;
 * }
 * @endcode
 */
class scoped_termination_guard_t {
public:
    scoped_termination_guard_t();
    ~scoped_termination_guard_t() noexcept(false);

    scoped_termination_guard_t(const scoped_termination_guard_t&) = delete;
    scoped_termination_guard_t& operator=(const scoped_termination_guard_t&) = delete;

private:
    std::array<struct sigaction, 3> m_previous_actions;
    std::array<bool, 3> m_active;
};

/**
 * @brief Temporarily forwards SIGINT, SIGTERM, and SIGHUP to one registered child or child process group.
 *
 * The callable constructor forks; the target-selection constructor only prepares
 * signal handling for a caller-managed fork. Only one child guard may be active
 * in a process; nested guards throw std::runtime_error. Signal handlers and
 * forwarding state are process-wide, and mask changes affect the calling thread.
 * Do not manage guards or handlers concurrently; multithreaded signal routing
 * is not provided.
 *
 * The first handled signal is recorded and forwarded; a second exits the parent
 * immediately with status 128 + signal number. Destruction restores the previous
 * handlers and mask, then throws termination_request_t for a recorded signal only
 * when no exception is already unwinding. Restoration failures exit the process.
 * Destruction neither waits for nor terminates the child. The caller owns waiting,
 * reaping, and any forced termination, including recovery after setup failures.
 * Keep the guard active through the wait and handle EINTR.
 *
 * @code{.cpp}
 * #include <m03gagbhsyhlx2pk5sdabbr1sx_signal_handler/signal_handler.h>
 * #include <cerrno>
 * #include <stdexcept>
 * #include <sys/wait.h>
 *
 * int main() {
 *     namespace signals = m03gagbhsyhlx2pk5sdabbr1sx_signal_handler;
 *     try {
 *         signals::scoped_child_termination_guard_t scoped_child_termination_guard([] {
 *             // Child work; returning exits this child with status zero.
 *         });
 *         int status = 0;
 *         while (waitpid(scoped_child_termination_guard.pid(), &status, 0) == -1) {
 *             if (errno != EINTR) {
 *                 throw std::runtime_error("failed to wait for child");
 *             }
 *         }
 *         return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
 *     } catch (const signals::termination_request_t& termination_request) {
 *         return 128 + termination_request.signal_number();
 *     }
 * }
 * @endcode
 */
class scoped_child_termination_guard_t {
public:
    /**
     * @brief Blocks termination signals and prepares forwarding without forking a child.
     *
     * Installs handlers while the calling thread's termination signals are blocked;
     * pid() remains -1. After fork(), call enter_child() in the child and
     * enter_parent() with the positive child pid in the parent. For process_group,
     * establish a group with id equal to the child pid before enter_parent().
     * Destruction restores state if no fork occurs. Setup failures, including a
     * nested guard, throw std::runtime_error.
     */
    explicit scoped_child_termination_guard_t(child_signal_target_t child_signal_target);

    /**
     * @brief Forks and invokes child_fn in the child while the parent forwards signals to its pid.
     *
     * The child restores inherited handlers and mask before invoking the callable;
     * it uses _exit(0) if the callable returns and _exit(127) if it throws.
     * Parent automatic objects are not unwound in the child by _exit(). The
     * callable and captures execute in the forked address space, not a new thread.
     * Setup or fork failures throw std::runtime_error in the parent.
     */
    template <class child_fn_t>
    explicit scoped_child_termination_guard_t(child_fn_t&& child_fn);

    ~scoped_child_termination_guard_t() noexcept(false);

    scoped_child_termination_guard_t(const scoped_child_termination_guard_t&) = delete;
    scoped_child_termination_guard_t& operator=(const scoped_child_termination_guard_t&) = delete;

    /** @brief Returns the registered child pid in the parent, or -1 before registration. */
    pid_t pid() const;

    /** @brief Restores inherited handlers and signal mask in the child after a caller-managed fork. */
    void enter_child();

    /**
     * @brief Registers the child pid and restores the parent's pre-setup signal mask to enable forwarding.
     *
     * Call once in the parent after a successful fork; pid must be positive.
     * For process_group, its process group must already exist with that id.
     * A mask restoration failure throws std::runtime_error after signal cleanup;
     * the caller still owns the child and must arrange its termination/reaping.
     */
    void enter_parent(pid_t pid);

private:
    void prepare(child_signal_target_t child_signal_target);
    pid_t fork_child();
    void cleanup_or_exit() noexcept;

private:
    pid_t m_pid;
    std::array<struct sigaction, 3> m_previous_actions;
    std::array<bool, 3> m_handler_active;
    sigset_t m_previous_mask;
    bool m_mask_active;
    bool m_registered;
};

} // namespace m03gagbhsyhlx2pk5sdabbr1sx_signal_handler

namespace m03gagbhsyhlx2pk5sdabbr1sx_signal_handler {

template <class child_fn_t>
scoped_child_termination_guard_t::scoped_child_termination_guard_t(child_fn_t&& child_fn):
    scoped_child_termination_guard_t(child_signal_target_t::process)
{
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
