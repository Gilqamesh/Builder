#include <m03gn97n4iusbtl7uthb01wu9m_test_framework/test_framework.h>
#include <m03gagbhsyhlx2pk5sdabbr1sx_signal_handler/signal_handler.h>

#include <functional>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <stdexcept>
#include <string>
#include <type_traits>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace api = m03gagbhsyhlx2pk5sdabbr1sx_signal_handler;
namespace test = m03gn97n4iusbtl7uthb01wu9m_test_framework;


namespace {

volatile sig_atomic_t g_custom_signal_count = 0;

void custom_signal_handler(int) {
    g_custom_signal_count = 1;
}

template <class fn_t>
int run_in_subprocess(fn_t&& fn) {
    const pid_t pid = fork();
    if (pid == -1) {
        throw std::runtime_error(std::string("fork failed: ") + std::strerror(errno));
    }

    if (pid == 0) {
        int exit_code = 255;
        try {
            exit_code = std::forward<fn_t>(fn)();
        } catch (...) {
            exit_code = 254;
        }
        _exit(exit_code);
    }

    int status = 0;
    if (waitpid(pid, &status, 0) == -1) {
        throw std::runtime_error(std::string("waitpid failed: ") + std::strerror(errno));
    }

    if (!WIFEXITED(status)) {
        return 253;
    }
    return WEXITSTATUS(status);
}

class default_signal_action_t {
public:
    explicit default_signal_action_t(int signal_number):
        m_signal_number(signal_number)
    {
        struct sigaction action {};
        action.sa_handler = SIG_DFL;
        sigemptyset(&action.sa_mask);
        action.sa_flags = 0;
        if (sigaction(signal_number, &action, &m_previous_action) == -1) {
            throw std::runtime_error("failed to install default signal action");
        }
    }

    ~default_signal_action_t() {
        sigaction(m_signal_number, &m_previous_action, nullptr);
    }

private:
    int m_signal_number;
    struct sigaction m_previous_action {};
};

void write_ready(int fd) {
    const char value = 'r';
    if (write(fd, &value, sizeof(value)) != sizeof(value)) {
        _exit(240);
    }
}

void read_ready(int fd) {
    char value = 0;
    const ssize_t count = read(fd, &value, sizeof(value));
    test::expect(std::equal_to<>(), count, ssize_t(1));
    test::expect(std::equal_to<>(), value, 'r');
}

} // namespace

int main() {
    return test::run([] {
        static_assert(!std::is_copy_constructible_v<api::scoped_termination_guard_t>);
        static_assert(!std::is_copy_assignable_v<api::scoped_termination_guard_t>);
        static_assert(!std::is_copy_constructible_v<api::scoped_child_termination_guard_t>);
        static_assert(!std::is_copy_assignable_v<api::scoped_child_termination_guard_t>);
        static_assert(api::child_signal_target_t::process != api::child_signal_target_t::process_group);

        const api::termination_request_t request(SIGTERM);
        test::expect(std::equal_to<>(), request.signal_number(), SIGTERM);
        test::expect(std::equal_to<>(), std::string(request.what()),
            std::string("termination requested by signal ") + std::to_string(SIGTERM)
        );

        test::expect(std::equal_to<>(), run_in_subprocess([] {
            struct sigaction custom_action {};
            custom_action.sa_handler = custom_signal_handler;
            sigemptyset(&custom_action.sa_mask);
            custom_action.sa_flags = 0;
            if (sigaction(SIGTERM, &custom_action, nullptr) == -1) {
                return 1;
            }

            g_custom_signal_count = 0;
            try {
                {
                    api::scoped_termination_guard_t guard;
                    if (raise(SIGTERM) != 0) {
                        return 2;
                    }
                }
                return 3;
            } catch (const api::termination_request_t& termination) {
                if (termination.signal_number() != SIGTERM) {
                    return 4;
                }
            }

            struct sigaction restored_action {};
            if (sigaction(SIGTERM, nullptr, &restored_action) == -1) {
                return 5;
            }
            if (restored_action.sa_handler != custom_signal_handler) {
                return 6;
            }
            if (g_custom_signal_count != 0) {
                return 7;
            }

            try {
                [[maybe_unused]] api::scoped_termination_guard_t second_guard;
                return 8;
            } catch (const api::termination_request_t& termination) {
                return termination.signal_number() == SIGTERM ? 0 : 9;
            }
        }), 0);

        test::expect(std::equal_to<>(), run_in_subprocess([] {
            struct sigaction previous_action {};
            struct sigaction custom_action {};
            custom_action.sa_handler = custom_signal_handler;
            sigemptyset(&custom_action.sa_mask);
            custom_action.sa_flags = 0;
            if (sigaction(SIGHUP, &custom_action, &previous_action) == -1) {
                return 1;
            }

            {
                api::scoped_termination_guard_t guard;
            }

            struct sigaction restored_action {};
            if (sigaction(SIGHUP, nullptr, &restored_action) == -1) {
                return 2;
            }
            if (restored_action.sa_handler != custom_signal_handler) {
                return 3;
            }
            sigaction(SIGHUP, &previous_action, nullptr);
            return 0;
        }), 0);

        test::expect(std::equal_to<>(), run_in_subprocess([] {
            try {
                api::scoped_termination_guard_t guard;
                if (raise(SIGINT) != 0) {
                    return 1;
                }
                throw std::logic_error("original exception");
            } catch (const std::logic_error&) {
                return 0;
            } catch (...) {
                return 2;
            }
        }), 0);

        {
            api::scoped_child_termination_guard_t guard(api::child_signal_target_t::process);
            test::expect(std::equal_to<>(), guard.pid(), pid_t(-1));
            test::expect_throws<std::runtime_error>([] {
                [[maybe_unused]] api::scoped_child_termination_guard_t nested(
                    api::child_signal_target_t::process
                );
            });
        }

        {
            int status = 0;
            pid_t child_pid = -1;
            {
                api::scoped_child_termination_guard_t guard([] {});
                child_pid = guard.pid();
                test::expect(std::identity(), 0 < child_pid);
                test::expect(std::equal_to<>(), waitpid(child_pid, &status, 0), child_pid);
            }
            test::expect(std::identity(), WIFEXITED(status));
            test::expect(std::equal_to<>(), WEXITSTATUS(status), 0);
        }

        {
            int status = 0;
            pid_t child_pid = -1;
            {
                api::scoped_child_termination_guard_t guard([] {
                    throw std::runtime_error("child failure");
                });
                child_pid = guard.pid();
                test::expect(std::equal_to<>(), waitpid(child_pid, &status, 0), child_pid);
            }
            test::expect(std::identity(), WIFEXITED(status));
            test::expect(std::equal_to<>(), WEXITSTATUS(status), 127);
        }

        {
            default_signal_action_t default_sigterm(SIGTERM);
            int ready_pipe[2];
            test::expect(std::equal_to<>(), pipe(ready_pipe), 0);

            int status = 0;
            pid_t child_pid = -1;
            bool caught = false;
            try {
                {
                    api::scoped_child_termination_guard_t guard([&] {
                        close(ready_pipe[0]);
                        write_ready(ready_pipe[1]);
                        close(ready_pipe[1]);
                        for (;;) {
                            pause();
                        }
                    });
                    child_pid = guard.pid();
                    close(ready_pipe[1]);
                    read_ready(ready_pipe[0]);
                    close(ready_pipe[0]);

                    test::expect(std::equal_to<>(), kill(getpid(), SIGTERM), 0);
                    test::expect(std::equal_to<>(), waitpid(child_pid, &status, 0), child_pid);
                }
            } catch (const api::termination_request_t& termination) {
                caught = true;
                test::expect(std::equal_to<>(), termination.signal_number(), SIGTERM);
            }

            test::expect(std::identity(), caught);
            test::expect(std::identity(), WIFSIGNALED(status));
            test::expect(std::equal_to<>(), WTERMSIG(status), SIGTERM);
        }

        {
            default_signal_action_t default_sighup(SIGHUP);
            int ready_pipe[2];
            test::expect(std::equal_to<>(), pipe(ready_pipe), 0);

            int status = 0;
            pid_t child_pid = -1;
            bool caught = false;
            try {
                {
                    api::scoped_child_termination_guard_t guard(
                        api::child_signal_target_t::process_group
                    );
                    test::expect(std::equal_to<>(), guard.pid(), pid_t(-1));

                    child_pid = fork();
                    test::expect(std::identity(), child_pid != -1);
                    if (child_pid == 0) {
                        close(ready_pipe[0]);
                        if (setpgid(0, 0) == -1) {
                            _exit(241);
                        }
                        guard.enter_child();
                        write_ready(ready_pipe[1]);
                        close(ready_pipe[1]);
                        for (;;) {
                            pause();
                        }
                    }

                    close(ready_pipe[1]);
                    if (setpgid(child_pid, child_pid) == -1 && getpgid(child_pid) != child_pid) {
                        throw std::runtime_error("failed to establish child process group");
                    }
                    guard.enter_parent(child_pid);
                    test::expect(std::equal_to<>(), guard.pid(), child_pid);
                    read_ready(ready_pipe[0]);
                    close(ready_pipe[0]);

                    test::expect(std::equal_to<>(), kill(getpid(), SIGHUP), 0);
                    test::expect(std::equal_to<>(), waitpid(child_pid, &status, 0), child_pid);
                }
            } catch (const api::termination_request_t& termination) {
                caught = true;
                test::expect(std::equal_to<>(), termination.signal_number(), SIGHUP);
            }

            test::expect(std::identity(), caught);
            test::expect(std::identity(), WIFSIGNALED(status));
            test::expect(std::equal_to<>(), WTERMSIG(status), SIGHUP);
        }
    });
}
