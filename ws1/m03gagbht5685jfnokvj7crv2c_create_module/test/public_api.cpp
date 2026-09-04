# include <m03gn97n4iusbtl7uthb01wu9m_test_framework/test_framework.h>
# include <m03gagbht5685jfnokvj7crv2c_create_module/create_module.h>

# include <chrono>
# include <cstdint>
# include <cstdlib>
# include <filesystem>
# include <format>
# include <fstream>
# include <functional>
# include <iterator>
# include <optional>
# include <stdexcept>
# include <string>

namespace api = m03gagbht5685jfnokvj7crv2c_create_module;
namespace filesystem_api = m03gagbhsnusi43zogoacgj2ez_filesystem;
namespace test = m03gn97n4iusbtl7uthb01wu9m_test_framework;


namespace {

class temporary_directory_t {
public:
    temporary_directory_t() {
        m_path = std::filesystem::temp_directory_path() / std::format(
            "builder-create-module-public-api-{}-{}",
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

std::string read_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("failed to read generated module file");
    }
    return std::string(
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>()
    );
}

std::string uppercase(std::string value) {
    for (char& character : value) {
        if ('a' <= character && character <= 'z') {
            character = static_cast<char>(character - 'a' + 'A');
        }
    }
    return value;
}

} // namespace

int main() {
    return test::run([] {
        temporary_directory_t temporary_directory;
        environment_guard_t workspace_root_guard("BUILDER_WORKSPACE_ROOT");
        environment_guard_t artifact_root_guard("BUILDER_ARTIFACT_ROOT");

        const auto workspace_root = temporary_directory.path() / "workspace-root";
        const auto artifact_root = temporary_directory.path() / "artifact-root";
        const auto workspace = workspace_root / "ws5";
        std::filesystem::create_directories(workspace);
        setenv("BUILDER_WORKSPACE_ROOT", workspace_root.c_str(), 1);
        setenv("BUILDER_ARTIFACT_ROOT", artifact_root.c_str(), 1);

        const auto created = api::create("ws5", "Example_Module9");
        test::expect(std::identity(), filesystem_api::is_directory(created));
        test::expect(std::equal_to<>(), created.parent(), filesystem_api::path_t(workspace));

        const auto module_name = created.filename();
        test::expect(std::identity(), module_name.ends_with("_Example_Module9"));
        test::expect(std::equal_to<>(), created.filename(), module_name);

        const auto api_header = created / filesystem_api::relative_path_t("api.h");
        const auto cli_source = created / filesystem_api::relative_path_t("cli.cpp");
        const auto builder_source = created / filesystem_api::relative_path_t("builder.cpp");
        test::expect(std::identity(), filesystem_api::is_regular_file(api_header));
        test::expect(std::identity(), filesystem_api::is_regular_file(cli_source));
        test::expect(std::identity(), filesystem_api::is_regular_file(builder_source));

        const auto header_contents = read_file(api_header.to_native_path());
        const auto expected_guard = uppercase(module_name) + "_API_H";
        test::expect(std::identity(), header_contents.find("#ifndef " + expected_guard) != std::string::npos);
        test::expect(std::identity(), header_contents.find("# define " + expected_guard) != std::string::npos);
        test::expect(std::identity(), header_contents.find("namespace " + module_name)
                != std::string::npos
        );
        test::expect(std::identity(), header_contents.find("#endif // " + expected_guard)
                != std::string::npos
        );

        const auto cli_contents = read_file(cli_source.to_native_path());
        test::expect(std::identity(), cli_contents.find("# include \"api.h\"") != std::string::npos);
        test::expect(std::identity(), cli_contents.find("# include <iostream>") != std::string::npos);
        test::expect(std::identity(), cli_contents.find("Hello from " + module_name + "!")
                != std::string::npos
        );
        test::expect(std::identity(), read_file(builder_source.to_native_path()).empty());

        const auto second = api::create("ws5", "Example_Module9");
        test::expect(std::identity(), filesystem_api::is_directory(second));
        test::expect(std::not_equal_to<>(), second, created);
        test::expect(std::equal_to<>(), second.filename().substr(second.filename().find('_') + 1),
            std::string("Example_Module9")
        );

        test::expect_throws<std::invalid_argument>([] {
            [[maybe_unused]] const auto path = api::create("ws5", "");
        });
        test::expect_throws<std::invalid_argument>([] {
            [[maybe_unused]] const auto path = api::create("ws5", "invalid-name");
        });
        test::expect_throws<std::runtime_error>([] {
            [[maybe_unused]] const auto path = api::create("missing", "Valid");
        });

        const auto workspace_file = workspace_root / "workspace-file";
        std::ofstream(workspace_file) << "not a directory";
        test::expect_throws<std::runtime_error>([] {
            [[maybe_unused]] const auto path = api::create("workspace-file", "Valid");
        });
        test::expect_throws<std::runtime_error>([] {
            [[maybe_unused]] const auto path = api::create("/absolute", "Valid");
        });
        std::filesystem::create_directories(workspace_root / "not-a-workspace");
        test::expect_throws<std::runtime_error>([] {
            [[maybe_unused]] const auto path = api::create("not-a-workspace", "Valid");
        });
        std::filesystem::create_directories(workspace / "nested");
        test::expect_throws<std::invalid_argument>([] {
            [[maybe_unused]] const auto path = api::create("ws5/nested", "Valid");
        });

        setenv("BUILDER_WORKSPACE_ROOT", "", 1);
        test::expect_throws<std::runtime_error>([] {
            [[maybe_unused]] const auto path = api::create("ws5", "Valid");
        });
    });
}
