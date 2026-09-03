#include <m03gn97n4iusbtl7uthb01wu9m_test_framework/test_framework.h>
#include <m03gagbht3svcx3ign454lfup3_cmake/cmake.h>

#include <functional>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

#include <sys/wait.h>
#include <unistd.h>

namespace api = m03gagbht3svcx3ign454lfup3_cmake;
namespace filesystem_api = m03gagbhsnusi43zogoacgj2ez_filesystem;
namespace test = m03gn97n4iusbtl7uthb01wu9m_test_framework;


namespace {

class temporary_directory_t {
public:
    temporary_directory_t() {
        m_path = std::filesystem::temp_directory_path() / std::format(
            "builder-cmake-public-api-{}-{}",
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
        throw std::runtime_error("failed to create CMake fixture");
    }
    output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
}

int run_binary(const std::filesystem::path& path) {
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
    test::expect(std::identity(), WIFEXITED(status));
    return WEXITSTATUS(status);
}

} // namespace

int main() {
    return test::run([] {
        temporary_directory_t temporary_directory;
        environment_guard_t ccache_dir_guard("CCACHE_DIR");
        environment_guard_t ccache_tmpdir_guard("CCACHE_TEMPDIR");
        const auto source_native = temporary_directory.path() / "source";
        const auto build_native = temporary_directory.path() / "build";
        const auto install_native = temporary_directory.path() / "install";
        const auto ccache_dir = temporary_directory.path() / "ccache";
        const auto ccache_tmpdir = ccache_dir / "tmp";
        std::filesystem::create_directories(ccache_tmpdir);
        setenv("CCACHE_DIR", ccache_dir.c_str(), 1);
        setenv("CCACHE_TEMPDIR", ccache_tmpdir.c_str(), 1);

        write_file(
            source_native / "CMakeLists.txt",
            "cmake_minimum_required(VERSION 3.16)\n"
            "project(builder_cmake_public_api LANGUAGES C)\n"
            "if(NOT DEFINED EXPECTED_VALUE)\n"
            "  message(FATAL_ERROR \"EXPECTED_VALUE was not provided\")\n"
            "endif()\n"
            "add_executable(cmake_fixture main.c)\n"
            "target_compile_definitions(cmake_fixture PRIVATE EXPECTED_VALUE=${EXPECTED_VALUE})\n"
            "install(TARGETS cmake_fixture DESTINATION bin)\n"
        );
        write_file(
            source_native / "main.c",
            "#ifndef EXPECTED_VALUE\n"
            "# error EXPECTED_VALUE is missing\n"
            "#endif\n"
            "int main(void) { return EXPECTED_VALUE == 42 ? 0 : 1; }\n"
        );

        const filesystem_api::path_t source(source_native);
        const filesystem_api::path_t build(build_native);
        const filesystem_api::path_t install(install_native);
        const filesystem_api::path_t missing(temporary_directory.path() / "missing");

        test::expect_throws<std::runtime_error>([&] {
            api::configure(missing, build, {});
        });
        test::expect_throws<std::runtime_error>([&] {
            api::build(missing, std::nullopt);
        });
        test::expect_throws<std::runtime_error>([&] {
            api::install(missing);
        });

        api::configure(
            source,
            build,
            {
                { "EXPECTED_VALUE", "42" },
                { "CMAKE_INSTALL_PREFIX", install.string() }
            }
        );
        test::expect(std::identity(), filesystem_api::is_directory(build));
        test::expect(std::identity(), filesystem_api::is_regular_file(
            build / filesystem_api::relative_path_t("CMakeCache.txt")
        ));

        api::build(build, std::nullopt);
        test::expect(std::identity(), filesystem_api::is_regular_file(
            build / filesystem_api::relative_path_t("cmake_fixture")
        ));
        api::build(build, std::size_t(1));

        api::install(build);
        const auto installed_binary = install_native / "bin" / "cmake_fixture";
        test::expect(std::identity(), std::filesystem::is_regular_file(installed_binary));
        test::expect(std::equal_to<>(), run_binary(installed_binary), 0);
    });
}
