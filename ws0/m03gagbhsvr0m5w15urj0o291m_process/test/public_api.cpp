#include <m03gn97n4iusbtl7uthb01wu9m_test_framework/test_framework.h>
#include <m03gagbhsvr0m5w15urj0o291m_process/process.h>

#include <functional>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

namespace api = m03gagbhsvr0m5w15urj0o291m_process;
namespace filesystem_api = m03gagbhsnusi43zogoacgj2ez_filesystem;
namespace test = m03gn97n4iusbtl7uthb01wu9m_test_framework;


namespace {

class temporary_directory_t {
public:
    temporary_directory_t() {
        m_path = std::filesystem::temp_directory_path() / std::format(
            "builder-process-public-api-{}-{}",
            std::chrono::steady_clock::now().time_since_epoch().count(),
            reinterpret_cast<std::uintptr_t>(this)
        );
        std::filesystem::create_directory(m_path);
    }

    ~temporary_directory_t() {
        std::error_code error;
        std::filesystem::remove_all(m_path, error);
    }

    const std::filesystem::path& path() const {
        return m_path;
    }

private:
    std::filesystem::path m_path;
};

std::string read_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("failed to open process test output");
    }
    return std::string(
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>()
    );
}

int wait_for(pid_t pid) {
    int status = 0;
    test::expect(std::equal_to<>(), waitpid(pid, &status, 0), pid);
    return status;
}

} // namespace

int main() {
    return test::run([] {
        const api::environment_variable_t variable("NAME", "value with spaces");
        test::expect(std::equal_to<>(), variable.name(), std::string("NAME"));
        test::expect(std::equal_to<>(), variable.value(), std::string("value with spaces"));
        test::expect(std::equal_to<>(), std::format("{}", variable),
            std::string("{ name: NAME, value: value with spaces }")
        );
        test::expect_throws<std::invalid_argument>([] {
            [[maybe_unused]] const api::environment_variable_t invalid("", "value");
        });
        test::expect_throws<std::invalid_argument>([] {
            [[maybe_unused]] const api::environment_variable_t invalid("A=B", "value");
        });

        const api::command_t empty_command({});
        test::expect(std::identity(), empty_command.args().empty());
        test::expect(std::identity(), !empty_command.working_dir());
        test::expect(std::identity(), empty_command.environment_variables().empty());
        test::expect(std::equal_to<>(), std::format("{}", empty_command),
            std::string("{ args: [], working_dir: null, environment_variables: [] }")
        );

        temporary_directory_t temporary_directory;
        const filesystem_api::path_t working_dir(temporary_directory.path());
        const api::command_t described_command(
            { "/bin/echo", "hello world" },
            working_dir,
            {
                api::environment_variable_t("FIRST", "one"),
                api::environment_variable_t("SECOND", "two")
            }
        );
        test::expect(std::identity(), described_command.args() == std::vector<std::string>({ "/bin/echo", "hello world" }));
        test::expect(std::identity(), described_command.working_dir().has_value());
        test::expect(std::equal_to<>(), *described_command.working_dir(), working_dir);
        test::expect(std::equal_to<>(), described_command.environment_variables().size(), std::size_t(2));
        test::expect(std::equal_to<>(), described_command.environment_variables()[0].name(), std::string("FIRST"));
        test::expect(std::equal_to<>(), described_command.environment_variables()[1].value(), std::string("two"));
        test::expect(std::equal_to<>(), std::format("{}", described_command),
            std::format(
                "{{ args: [/bin/echo, hello world], working_dir: {}, environment_variables: [{{ name: FIRST, value: one }}, {{ name: SECOND, value: two }}] }}",
                working_dir
            )
        );

        test::expect(std::equal_to<>(), api::create_and_wait(api::command_t({ "/bin/true" })), 0);
        test::expect(std::equal_to<>(), api::create_and_wait(api::command_t({ "/bin/sh", "-c", "exit 7" })),
            7
        );
        test::expect(std::equal_to<>(), api::create_and_wait(api::command_t({ "/bin/sh", "-c", "kill -TERM $$" })),
            -SIGTERM
        );
        test::expect(std::equal_to<>(), api::create_and_wait(api::command_t({ "/definitely/not/an/executable" })),
            127
        );
        test::expect(std::equal_to<>(), api::create_and_wait(empty_command), 127);

        const auto output_path = temporary_directory.path() / "child-output";
        const std::string script = std::format(
            "printf '%s\\n%s\\n%s\\n' \"$PROCESS_TEST_VALUE\" \"$PWD\" \"$1\" > '{}'",
            output_path.string()
        );
        api::create_and_wait_checked(api::command_t(
            { "/bin/sh", "-c", script, "process-test", "argument with spaces" },
            working_dir,
            {
                api::environment_variable_t("PROCESS_TEST_VALUE", "first"),
                api::environment_variable_t("PROCESS_TEST_VALUE", "second")
            }
        ));
        test::expect(std::equal_to<>(), read_file(output_path),
            std::string("second\n") + working_dir.string() + "\nargument with spaces\n"
        );

        test::expect_no_throw([] {
            api::create_and_wait_checked(api::command_t({ "/bin/true" }));
        });
        test::expect_throws<std::runtime_error>([] {
            api::create_and_wait_checked(api::command_t({ "/bin/sh", "-c", "exit 11" }));
        });
        test::expect_throws<std::runtime_error>([] {
            api::create_and_wait_checked(api::command_t({ "/bin/sh", "-c", "kill -HUP $$" }));
        });
        test::expect_throws<std::runtime_error>([&] {
            api::create_and_wait_checked(api::command_t(
                { "/bin/true" },
                filesystem_api::path_t(temporary_directory.path() / "missing-working-directory")
            ));
        });

        test::expect_no_throw([] {
            api::create_and_wait_foreground_checked(api::command_t({ "/bin/true" }));
        });
        test::expect_throws<std::runtime_error>([] {
            api::create_and_wait_foreground_checked(
                api::command_t({ "/bin/sh", "-c", "exit 9" })
            );
        });
        test::expect_throws<std::runtime_error>([] {
            api::create_and_wait_foreground_checked(
                api::command_t({ "/bin/sh", "-c", "kill -TERM $$" })
            );
        });

        {
            const pid_t child = fork();
            test::expect(std::identity(), child != -1);
            if (child == 0) {
                api::exec(api::command_t({ "/bin/sh", "-c", "exit 23" }));
            }
            const int status = wait_for(child);
            test::expect(std::identity(), WIFEXITED(status));
            test::expect(std::equal_to<>(), WEXITSTATUS(status), 23);
        }

        {
            const pid_t child = fork();
            test::expect(std::identity(), child != -1);
            if (child == 0) {
                try {
                    api::exec(api::command_t({}));
                } catch (const std::runtime_error&) {
                    _exit(0);
                }
                _exit(1);
            }
            const int status = wait_for(child);
            test::expect(std::identity(), WIFEXITED(status));
            test::expect(std::equal_to<>(), WEXITSTATUS(status), 0);
        }

        {
            const pid_t child = fork();
            test::expect(std::identity(), child != -1);
            if (child == 0) {
                try {
                    api::exec(api::command_t({ "/missing-program" }));
                } catch (const std::runtime_error&) {
                    _exit(0);
                }
                _exit(1);
            }
            const int status = wait_for(child);
            test::expect(std::identity(), WIFEXITED(status));
            test::expect(std::equal_to<>(), WEXITSTATUS(status), 0);
        }
    });
}
