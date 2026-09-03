#include <m03gn97n4iusbtl7uthb01wu9m_test_framework/test_framework.h>
#include <m03gagbhst621faiop1rztfkqp_builder_cli/builder_cli.h>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

namespace api = m03gagbhst621faiop1rztfkqp_builder_cli;
namespace graph_api = m03gagbhsp2drqq3gkop8pzfrm_workspace_graph;
namespace test = m03gn97n4iusbtl7uthb01wu9m_test_framework;


namespace {

const std::vector<std::string> bootstrap_module_names {
    "m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain",
    "m03gagbhsnusi43zogoacgj2ez_filesystem",
    "m03gagbhsp2drqq3gkop8pzfrm_workspace_graph",
    "m03gagbhst621faiop1rztfkqp_builder_cli",
    "m03gagbhsujjf63n0w3r2w4q6h_build_phases",
    "m03gagbhsvr0m5w15urj0o291m_process",
    "m03gagbhsyhlx2pk5sdabbr1sx_signal_handler",
    "m03gagbhsx4j5z28bqkac3dhhh_shared_library",
    "m03gagbht2l61mj6qitacwbmea_byte_stream",
    "m03gagbhtft23yhjwpp881tfmc_uuid",
    "m03gn7qllwpi68ovctow4jrccj_lexer",
    "m03gn8rf3pe8dkpk1uwsemhhmd_artifact_store",
    "m03gn8rf3peew0re4l1s2vvaw6_bootstrap_seed",
    "m03gn8rf3pe86v64vphnaam6rl_source_dependencies"
};

class temporary_directory_t {
public:
    temporary_directory_t() {
        m_path = std::filesystem::temp_directory_path() / std::format(
            "builder-cli-public-api-{}-{}",
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

class environment_guard_t {
public:
    explicit environment_guard_t(const char* name):
        m_name(name)
    {
        if (const char* value = std::getenv(name); value != nullptr) {
            m_value = value;
        }
    }

    ~environment_guard_t() {
        if (m_value) {
            setenv(m_name.c_str(), m_value->c_str(), 1);
        } else {
            unsetenv(m_name.c_str());
        }
    }

private:
    std::string m_name;
    std::optional<std::string> m_value;
};

void write_file(const std::filesystem::path& path, std::string_view contents) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary);
    if (!output) {
        throw std::runtime_error("failed to create builder-cli fixture");
    }
    output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
}

void build_empty_plugin(const std::filesystem::path& path) {
    const auto source = path.parent_path() / "empty-builder.c";
    write_file(source, "int empty_builder_plugin;\n");
    const auto command = std::format(
        "cc -shared -fPIC -o '{}' '{}'",
        path.string(),
        source.string()
    );
    if (std::system(command.c_str()) != 0) {
        throw std::runtime_error("failed to compile empty builder plugin");
    }
}

void set_tree_write_time(
    const std::filesystem::path& root,
    std::filesystem::file_time_type time
) {
    for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
        std::filesystem::last_write_time(entry.path(), time);
    }
    std::filesystem::last_write_time(root, time);
}

int run_in_subprocess(const std::function<void()>& function) {
    const pid_t pid = fork();
    if (pid == -1) {
        throw std::runtime_error("fork failed");
    }
    if (pid == 0) {
        try {
            function();
            _exit(250);
        } catch (...) {
            _exit(251);
        }
    }

    int status = 0;
    test::expect(std::equal_to<>(), waitpid(pid, &status, 0), pid);
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    if (WIFSIGNALED(status)) {
        return 128 + WTERMSIG(status);
    }
    return 252;
}

std::vector<std::string> read_lines(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("target CLI did not create its output file");
    }

    std::vector<std::string> result;
    for (std::string line; std::getline(input, line);) {
        result.push_back(std::move(line));
    }
    return result;
}

void expect_cli_output(
    const std::filesystem::path& path,
    const std::vector<std::string>& arguments
) {
    const auto lines = read_lines(path);
    test::expect(std::equal_to<>(), lines.size(), arguments.size() + 3);
    test::expect(std::equal_to<>(), lines[0], std::to_string(arguments.size() + 1));
    test::expect(std::identity(), !lines[1].empty());
    for (std::size_t i = 0; i < arguments.size(); ++i) {
        test::expect(std::equal_to<>(), lines[i + 2], arguments[i]);
    }
    test::expect(std::identity(), std::filesystem::equivalent(
        std::filesystem::path(lines.back()),
        std::filesystem::path(lines[1]).parent_path()
    ));
}

} // namespace

int main() {
    return test::run([] {
        temporary_directory_t temporary_directory;
        environment_guard_t workspace_root_guard("BUILDER_WORKSPACE_ROOT");
        environment_guard_t artifact_root_guard("BUILDER_ARTIFACT_ROOT");

        const auto workspace_root = temporary_directory.path() / "workspace";
        const auto artifact_root = temporary_directory.path() / "artifacts";
        const graph_api::module_name_t target_module(
            "m03gagbhsvr0m5w15urj0o291m_process"
        );

        for (const auto& module_name : bootstrap_module_names) {
            std::filesystem::create_directories(workspace_root / "ws0" / module_name);
        }

        const auto target_dir = workspace_root / "ws0" / target_module.unique_name();
        write_file(
            target_dir / "cli.cpp",
            "#include <filesystem>\n"
            "#include <fstream>\n"
            "#include <string>\n"
            "int main(int argc, char** argv) {\n"
            "    if (argc < 2) return 90;\n"
            "    std::ofstream output(argv[1]);\n"
            "    if (!output) return 91;\n"
            "    output << argc << '\\n';\n"
            "    for (int i = 0; i < argc; ++i) output << argv[i] << '\\n';\n"
            "    output << std::filesystem::current_path().string() << '\\n';\n"
            "    if (2 < argc && std::string(argv[2]) == \"fail\") return 9;\n"
            "    return 0;\n"
            "}\n"
        );

        const auto old_time = std::filesystem::file_time_type::clock::now()
            - std::chrono::hours(24 * 365 * 20);
        for (const auto& module_name : bootstrap_module_names) {
            set_tree_write_time(workspace_root / "ws0" / module_name, old_time);
        }

        const graph_api::module_name_t builder_cli_module(
            "m03gagbhst621faiop1rztfkqp_builder_cli"
        );
        const auto plugin = artifact_root
            / builder_cli_module.unique_name()
            / "latest"
            / "builder"
            / "install"
            / "builder.so";
        std::filesystem::create_directories(plugin.parent_path());
        build_empty_plugin(plugin);

        setenv("BUILDER_WORKSPACE_ROOT", workspace_root.c_str(), 1);
        setenv("BUILDER_ARTIFACT_ROOT", artifact_root.c_str(), 1);

        const auto output_1 = temporary_directory.path() / "exec-argc.txt";
        const std::string output_1_string = output_1.string();
        const char* exec_argc_args[] = {
            output_1_string.c_str(),
            "argc-overload",
            "two words"
        };
        test::expect(std::equal_to<>(), run_in_subprocess([&] {
            api::exec(target_module, 3, exec_argc_args);
        }), 0);
        expect_cli_output(output_1, { output_1_string, "argc-overload", "two words" });

        const auto output_2 = temporary_directory.path() / "exec-target-argc.txt";
        const std::string output_2_string = output_2.string();
        const char* exec_target_argc_args[] = {
            output_2_string.c_str(),
            "target-argc"
        };
        test::expect(std::equal_to<>(), run_in_subprocess([&] {
            api::exec(target_module, "cli", 2, exec_target_argc_args);
        }), 0);
        expect_cli_output(output_2, { output_2_string, "target-argc" });

        const auto output_3 = temporary_directory.path() / "exec-vector.txt";
        const std::string output_3_string = output_3.string();
        test::expect(std::equal_to<>(), run_in_subprocess([&] {
            api::exec(target_module, std::vector<std::string> {
                output_3_string,
                "vector-overload"
            });
        }), 0);
        expect_cli_output(output_3, { output_3_string, "vector-overload" });

        const auto output_4 = temporary_directory.path() / "exec-target-vector.txt";
        const std::string output_4_string = output_4.string();
        test::expect(std::equal_to<>(), run_in_subprocess([&] {
            api::exec(target_module, "cli", std::vector<std::string> {
                output_4_string,
                "target-vector"
            });
        }), 0);
        expect_cli_output(output_4, { output_4_string, "target-vector" });

        const auto output_5 = temporary_directory.path() / "wait-argc.txt";
        const std::string output_5_string = output_5.string();
        const char* wait_argc_args[] = {
            output_5_string.c_str(),
            "wait-argc"
        };
        api::create_and_wait_checked(target_module, 2, wait_argc_args);
        expect_cli_output(output_5, { output_5_string, "wait-argc" });

        const auto output_6 = temporary_directory.path() / "wait-target-argc.txt";
        const std::string output_6_string = output_6.string();
        const char* wait_target_argc_args[] = {
            output_6_string.c_str(),
            "wait-target-argc"
        };
        api::create_and_wait_checked(target_module, "cli", 2, wait_target_argc_args);
        expect_cli_output(output_6, { output_6_string, "wait-target-argc" });

        const auto output_7 = temporary_directory.path() / "wait-vector.txt";
        const std::string output_7_string = output_7.string();
        api::create_and_wait_checked(target_module, std::vector<std::string> {
            output_7_string,
            "wait-vector"
        });
        expect_cli_output(output_7, { output_7_string, "wait-vector" });

        const auto output_8 = temporary_directory.path() / "wait-target-vector.txt";
        const std::string output_8_string = output_8.string();
        api::create_and_wait_checked(target_module, "cli", std::vector<std::string> {
            output_8_string,
            "wait-target-vector"
        });
        expect_cli_output(output_8, { output_8_string, "wait-target-vector" });

        const auto failure_output = temporary_directory.path() / "failure.txt";
        test::expect_throws<std::runtime_error>([&] {
            api::create_and_wait_checked(target_module, std::vector<std::string> {
                failure_output.string(),
                "fail"
            });
        });
        expect_cli_output(failure_output, { failure_output.string(), "fail" });

        test::expect_throws<std::runtime_error>([&] {
            api::create_and_wait_checked(
                target_module,
                "bad/name",
                std::vector<std::string> { failure_output.string() }
            );
        });
        test::expect_throws<std::runtime_error>([&] {
            api::exec(
                target_module,
                "bad/name",
                std::vector<std::string> { failure_output.string() }
            );
        });
    });
}
