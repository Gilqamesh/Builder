#ifndef M03GAGBHSVR0M5W15URJ0O291M_PROCESS_FOREGROUND_JOB_H
# define M03GAGBHSVR0M5W15URJ0O291M_PROCESS_FOREGROUND_JOB_H

# include "process.h"

# include <format>

# include <sys/types.h>
# include <termios.h>

# include <m03gagbhsyhlx2pk5sdabbr1sx_signal_handler/signal_handler.h>

namespace m03gagbhsvr0m5w15urj0o291m_process {

/**
 * @brief Owns foreground terminal handoff and restoration for one forked child process.
 */
class foreground_job_t {
public:
    /**
     * @brief Runs command as the terminal foreground job and returns its process result.
     */
    static int create_and_wait(const command_t& command);

    /**
     * @brief Returns whether stdin has an active foreground job.
     */
    bool active() const;

    /**
     * @brief Returns the tracked child process id.
     */
    pid_t child_pid() const;

    /**
     * @brief Returns the process group that owned the terminal before the child.
     */
    pid_t previous_foreground_process_group() const;

    /**
     * @brief Returns whether terminal attributes were saved.
     */
    bool terminal_attributes_saved() const;

    /**
     * @brief Returns whether terminal foreground ownership was transferred.
     */
    bool foreground_transferred() const;

    /**
     * @brief Returns the child handoff gate read descriptor.
     */
    int child_gate_read_descriptor() const;

    /**
     * @brief Returns the child handoff gate write descriptor.
     */
    int child_gate_write_descriptor() const;

private:
    /**
     * @brief Creates an active foreground job when stdin is a terminal.
     */
    foreground_job_t();

    /**
     * @brief Restores terminal state without throwing.
     */
    ~foreground_job_t();

    foreground_job_t(const foreground_job_t&) = delete;
    foreground_job_t& operator=(const foreground_job_t&) = delete;
    foreground_job_t(foreground_job_t&&) = delete;
    foreground_job_t& operator=(foreground_job_t&&) = delete;

    /**
     * @brief Returns the waitpid options needed while the child owns the terminal.
     */
    int wait_options() const;

    /**
     * @brief Returns whether termination signals should target the child process or process group.
     */
    m03gagbhsyhlx2pk5sdabbr1sx_signal_handler::child_signal_target_t child_signal_target() const;

    /**
     * @brief Moves the child into its own process group before exec.
     */
    void prepare_child();

    /**
     * @brief Gives terminal foreground ownership to the child process group.
     */
    void prepare_parent(pid_t child_pid);

    /**
     * @brief Restores terminal foreground ownership and saved attributes.
     */
    void restore();

    /**
     * @brief Kills the child process group, falling back to the child process.
     */
    void kill_child() const noexcept;

    /**
     * @brief Restores any remaining terminal state during destruction.
     */
    void restore_noexcept() noexcept;

private:
    bool m_active;
    pid_t m_child_pid;
    pid_t m_previous_foreground_process_group;
    struct termios m_saved_terminal_attributes;
    bool m_terminal_attributes_saved;
    bool m_foreground_transferred;
    int m_child_gate_read_descriptor;
    int m_child_gate_write_descriptor;
};

} // namespace m03gagbhsvr0m5w15urj0o291m_process

namespace std {

template <>
struct formatter<m03gagbhsvr0m5w15urj0o291m_process::foreground_job_t>;

} // namespace std

namespace std {

template <>
struct formatter<m03gagbhsvr0m5w15urj0o291m_process::foreground_job_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();

        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid foreground_job_t format specifier");
        }

        return it;
    }

    auto format(const m03gagbhsvr0m5w15urj0o291m_process::foreground_job_t& foreground_job, auto& ctx) const {
        auto out = ctx.out();

        out = std::format_to(out, "{{ ");
        out = std::format_to(out, "active: {}, ", foreground_job.active());
        out = std::format_to(out, "child_pid: {}, ", foreground_job.child_pid());
        out = std::format_to(out, "previous_foreground_process_group: {}, ", foreground_job.previous_foreground_process_group());
        out = std::format_to(out, "terminal_attributes_saved: {}, ", foreground_job.terminal_attributes_saved());
        out = std::format_to(out, "foreground_transferred: {}, ", foreground_job.foreground_transferred());
        out = std::format_to(out, "child_gate_read_descriptor: {}, ", foreground_job.child_gate_read_descriptor());
        out = std::format_to(out, "child_gate_write_descriptor: {}", foreground_job.child_gate_write_descriptor());
        out = std::format_to(out, " }}");

        return out;
    }
};

} // namespace std

#endif // M03GAGBHSVR0M5W15URJ0O291M_PROCESS_FOREGROUND_JOB_H
