#include <m03gn97n4iusbtl7uthb01wu9m_test_framework/test_framework.h>
#include <m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain/cxx_toolchain.h>

#include <functional>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <dlfcn.h>
#include <filesystem>
#include <format>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

namespace api = m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain;
namespace filesystem_api = m03gagbhsnusi43zogoacgj2ez_filesystem;
namespace test = m03gn97n4iusbtl7uthb01wu9m_test_framework;


namespace {

class temporary_directory_t {
public:
    temporary_directory_t() {
        m_path = std::filesystem::temp_directory_path() / std::format(
            "builder-cxx-toolchain-public-api-{}-{}",
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

class scoped_standard_error_redirect_t {
public:
    scoped_standard_error_redirect_t() {
        m_saved_fd = dup(STDERR_FILENO);
        if (m_saved_fd == -1) {
            throw std::runtime_error("failed to duplicate stderr");
        }

        const int null_fd = open("/dev/null", O_WRONLY);
        if (null_fd == -1) {
            close(m_saved_fd);
            throw std::runtime_error("failed to open /dev/null");
        }

        if (dup2(null_fd, STDERR_FILENO) == -1) {
            close(null_fd);
            close(m_saved_fd);
            throw std::runtime_error("failed to redirect stderr");
        }
        close(null_fd);
    }

    ~scoped_standard_error_redirect_t() {
        if (m_saved_fd != -1) {
            dup2(m_saved_fd, STDERR_FILENO);
            close(m_saved_fd);
        }
    }

    scoped_standard_error_redirect_t(const scoped_standard_error_redirect_t&) = delete;
    scoped_standard_error_redirect_t& operator=(const scoped_standard_error_redirect_t&) = delete;

private:
    int m_saved_fd = -1;
};

void write_file(const std::filesystem::path& path, std::string_view contents) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary);
    if (!output) {
        throw std::runtime_error("failed to create toolchain test source");
    }
    output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
}

int run_binary(const filesystem_api::path_t& path) {
    const pid_t pid = fork();
    if (pid == -1) {
        throw std::runtime_error("fork failed");
    }
    if (pid == 0) {
        execl(path.c_str(), path.c_str(), static_cast<char*>(nullptr));
        _exit(127);
    }

    int status = 0;
    test::expect(std::equal_to<>(), waitpid(pid, &status, 0), pid);
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    if (WIFSIGNALED(status)) {
        return -WTERMSIG(status);
    }
    return 255;
}

} // namespace

int main() {
    return test::run([] {
        const api::define_t define("_VALID_9", "value\"with\\escapes");
        test::expect(std::equal_to<>(), define.key(), std::string("_VALID_9"));
        test::expect(std::equal_to<>(), define.value(), std::string("value\"with\\escapes"));
        test::expect_throws<std::runtime_error>([] {
            [[maybe_unused]] const api::define_t invalid("", "value");
        });
        test::expect_throws<std::runtime_error>([] {
            [[maybe_unused]] const api::define_t invalid("9INVALID", "value");
        });
        test::expect_throws<std::runtime_error>([] {
            [[maybe_unused]] const api::define_t invalid("INVALID-NAME", "value");
        });
        test::expect_no_throw([] {
            [[maybe_unused]] const api::define_t valid("A", "");
            [[maybe_unused]] const api::define_t valid_with_digits("A0_b9", "x");
        });

        api::link_inputs_t empty_link_inputs;
        test::expect(std::identity(), empty_link_inputs.libraries.empty());

        temporary_directory_t temporary_directory;
        const auto source_root_native = temporary_directory.path() / "source";
        const auto include_root_native = temporary_directory.path() / "include";
        const auto library_build_native = temporary_directory.path() / "build" / "library";
        const auto binary_build_native = temporary_directory.path() / "build" / "binary";
        const auto library_output_native = temporary_directory.path() / "output" / "libfixture.so";
        const auto binary_output_native = temporary_directory.path() / "output" / "fixture_binary";

        write_file(
            include_root_native / "fixture.hpp",
            "#pragma once\n"
            "extern \"C\" int combined_value();\n"
            "extern \"C\" const char* configured_message();\n"
        );
        write_file(
            source_root_native / "c" / "value.c",
            "int c_value(void) { return 5; }\n"
        );
        write_file(
            source_root_native / "cpp" / "library.cpp",
            "#include <fixture.hpp>\n"
            "extern \"C\" int c_value();\n"
            "#ifndef TEST_MESSAGE\n"
            "# error TEST_MESSAGE is required\n"
            "#endif\n"
            "extern \"C\" int combined_value() { return c_value() + 37; }\n"
            "extern \"C\" const char* configured_message() { return TEST_MESSAGE; }\n"
        );
        write_file(
            source_root_native / "app" / "main.cpp",
            "#include <fixture.hpp>\n"
            "int main() { return combined_value() == 42 ? 0 : 1; }\n"
        );

        const filesystem_api::path_t source_root(source_root_native);
        const filesystem_api::path_t include_root(include_root_native);
        const filesystem_api::rooted_path_t c_source(
            source_root,
            filesystem_api::relative_path_t("c/value.c")
        );
        const filesystem_api::rooted_path_t cpp_source(
            source_root,
            filesystem_api::relative_path_t("cpp/library.cpp")
        );
        const filesystem_api::rooted_path_t binary_source(
            source_root,
            filesystem_api::relative_path_t("app/main.cpp")
        );

        const filesystem_api::path_t library_build(library_build_native);
        const filesystem_api::path_t binary_build(binary_build_native);
        const filesystem_api::path_t library_output(library_output_native);
        const filesystem_api::path_t binary_output(binary_output_native);
        const std::string message = "hello \"builder\" \\ path";

        const auto built_library = api::build_library(
            library_build,
            { include_root },
            { c_source, cpp_source },
            { api::define_t("TEST_MESSAGE", message) },
            empty_link_inputs,
            library_output
        );
        test::expect(std::equal_to<>(), built_library, library_output);
        test::expect(std::identity(), filesystem_api::is_regular_file(built_library));
        test::expect(std::identity(), filesystem_api::is_regular_file(
            library_build / filesystem_api::relative_path_t("c/value.o")
        ));
        test::expect(std::identity(), filesystem_api::is_regular_file(
            library_build / filesystem_api::relative_path_t("cpp/library.o")
        ));

        void* handle = dlopen(built_library.c_str(), RTLD_NOW | RTLD_LOCAL);
        test::expect(std::identity(), handle != nullptr);
        using value_fn_t = int (*)();
        using message_fn_t = const char* (*)();
        auto combined_value = reinterpret_cast<value_fn_t>(dlsym(handle, "combined_value"));
        auto configured_message = reinterpret_cast<message_fn_t>(dlsym(handle, "configured_message"));
        test::expect(std::identity(), combined_value != nullptr);
        test::expect(std::identity(), configured_message != nullptr);
        test::expect(std::equal_to<>(), combined_value(), 42);
        test::expect(std::equal_to<>(), std::string(configured_message()), message);
        test::expect(std::equal_to<>(), dlclose(handle), 0);

        api::link_inputs_t link_inputs {
            .libraries = { built_library }
        };
        test::expect(std::equal_to<>(), link_inputs.libraries.size(), std::size_t(1));
        test::expect(std::equal_to<>(), link_inputs.libraries[0], built_library);

        const auto built_binary = api::build_binary(
            binary_build,
            { include_root },
            { binary_source },
            {},
            link_inputs,
            binary_output
        );
        test::expect(std::equal_to<>(), built_binary, binary_output);
        test::expect(std::identity(), filesystem_api::is_regular_file(built_binary));
        test::expect(std::identity(), filesystem_api::is_regular_file(
            binary_build / filesystem_api::relative_path_t("app/main.o")
        ));
        test::expect(std::equal_to<>(), run_binary(built_binary), 0);

        write_file(source_root_native / "gone.cpp", "int gone;\n");
        const filesystem_api::rooted_path_t gone_source(
            source_root,
            filesystem_api::relative_path_t("gone.cpp")
        );
        std::filesystem::remove(source_root_native / "gone.cpp");
        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto output = api::build_library(
                filesystem_api::path_t(temporary_directory.path() / "gone-build"),
                {},
                { gone_source },
                {},
                {},
                filesystem_api::path_t(temporary_directory.path() / "gone-output.so")
            );
        });

        write_file(source_root_native / "bad.cpp", "this is not valid C++\n");
        const filesystem_api::rooted_path_t bad_source(
            source_root,
            filesystem_api::relative_path_t("bad.cpp")
        );
        {
            scoped_standard_error_redirect_t stderr_redirect;
            test::expect_throws<std::runtime_error>([&] {
                [[maybe_unused]] const auto output = api::build_binary(
                    filesystem_api::path_t(temporary_directory.path() / "bad-build"),
                    {},
                    { bad_source },
                    {},
                    {},
                    filesystem_api::path_t(temporary_directory.path() / "bad-output")
                );
            });
        }

        const api::link_inputs_t missing_link_input {
            .libraries = {
                filesystem_api::path_t(temporary_directory.path() / "missing-library.so")
            }
        };
        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto output = api::build_binary(
                filesystem_api::path_t(temporary_directory.path() / "missing-link-build"),
                { include_root },
                { binary_source },
                {},
                missing_link_input,
                filesystem_api::path_t(temporary_directory.path() / "missing-link-output")
            );
        });
    });
}
