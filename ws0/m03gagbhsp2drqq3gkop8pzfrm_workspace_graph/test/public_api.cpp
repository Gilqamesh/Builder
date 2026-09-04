#include <m03gn97n4iusbtl7uthb01wu9m_test_framework/test_framework.h>
#include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>

#include <functional>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <type_traits>

namespace api = m03gagbhsp2drqq3gkop8pzfrm_workspace_graph;
namespace filesystem_api = m03gagbhsnusi43zogoacgj2ez_filesystem;
namespace test = m03gn97n4iusbtl7uthb01wu9m_test_framework;


namespace {

class temporary_directory_t {
public:
    temporary_directory_t() {
        m_path = std::filesystem::temp_directory_path() / std::format(
            "builder-workspace-graph-public-api-{}-{}",
            std::chrono::steady_clock::now().time_since_epoch().count(),
            reinterpret_cast<std::uintptr_t>(this)
        );

        std::error_code error;
        const bool created = std::filesystem::create_directory(m_path, error);
        if (error || !created) {
            throw std::runtime_error("failed to create temporary test directory");
        }
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

class current_path_guard_t {
public:
    current_path_guard_t():
        m_path(std::filesystem::current_path())
    {
    }

    ~current_path_guard_t() {
        std::error_code error;
        std::filesystem::current_path(m_path, error);
    }

private:
    std::filesystem::path m_path;
};

void create_file(const std::filesystem::path& path, std::string_view contents = {}) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary);
    if (!output) {
        throw std::runtime_error("failed to create test file");
    }
    output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
}

} // namespace

int main() {
    return test::run([] {
        test::expect(std::equal_to<>(), std::string(api::BUILDER_CPP), std::string("builder.cpp"));
        test::expect(std::equal_to<>(), std::string(api::CLI_CPP), std::string("cli.cpp"));

        const api::version_t direct_version(42);
        test::expect(std::equal_to<>(), direct_version.value, std::uint64_t(42));

        const auto file_time = std::filesystem::file_time_type::clock::now();
        using file_time_rep_t = std::filesystem::file_time_type::duration::rep;
        using unsigned_file_time_rep_t = std::make_unsigned_t<file_time_rep_t>;
        const auto expected_file_time_version = static_cast<std::uint64_t>(
            static_cast<unsigned_file_time_rep_t>(file_time.time_since_epoch().count())
            - static_cast<unsigned_file_time_rep_t>(std::numeric_limits<file_time_rep_t>::min())
        );
        test::expect(std::equal_to<>(), api::version_t(file_time).value, expected_file_time_version);
        const auto minimum_file_time = std::filesystem::file_time_type(
            std::filesystem::file_time_type::duration(std::numeric_limits<file_time_rep_t>::min())
        );
        test::expect(std::equal_to<>(), api::version_t(minimum_file_time).value, std::uint64_t(0));

        const std::string known_unique_name = "m03gagbht2l61mj6qitacwbmea_byte_stream";
        const api::module_name_t known_name(known_unique_name);
        test::expect(std::equal_to<>(), known_name.unique_name(), known_unique_name);
        test::expect(std::equal_to<>(), known_name.friendly_name(), std::string("byte_stream"));
        test::expect(std::equal_to<>(), known_name.uuid().version(), 7U);
        test::expect(std::equal_to<>(), known_name.uuid().bytes().size(), std::size_t(16));
        test::expect(std::equal_to<>(), std::format("{}", known_name), known_unique_name);
        test::expect(std::equal_to<>(), api::module_name_hash_t()(known_name),
            api::module_name_hash_t()(api::module_name_t(known_unique_name))
        );

        const auto generated_name = api::module_name_t::from_friendly_name("Friendly_7");
        test::expect(std::equal_to<>(), generated_name.friendly_name(), std::string("Friendly_7"));
        test::expect(std::equal_to<>(), generated_name.unique_name().size(), std::size_t(37));
        test::expect(std::equal_to<>(), generated_name.unique_name()[0], 'm');
        test::expect(std::equal_to<>(), generated_name.unique_name()[26], '_');
        test::expect(std::equal_to<>(), generated_name.uuid().version(), 7U);
        test::expect(std::equal_to<>(), api::module_name_t(generated_name.unique_name()), generated_name);

        const api::module_name_t later_name("m03gagbhtft23yhjwpp881tfmc_uuid");
        test::expect(std::identity(), known_name == api::module_name_t(known_unique_name));
        test::expect(std::identity(), (known_name < later_name) || (later_name < known_name));
        test::expect(std::identity(), known_name <= known_name);

        test::expect_throws<std::invalid_argument>([] {
            [[maybe_unused]] const api::module_name_t invalid("");
        });
        test::expect_throws<std::invalid_argument>([&] {
            auto value = known_unique_name;
            value[0] = 'x';
            [[maybe_unused]] const api::module_name_t invalid(value);
        });
        test::expect_throws<std::invalid_argument>([&] {
            auto value = known_unique_name;
            value[1] = '!';
            [[maybe_unused]] const api::module_name_t invalid(value);
        });
        test::expect_throws<std::invalid_argument>([&] {
            auto value = known_unique_name;
            value[26] = 'x';
            [[maybe_unused]] const api::module_name_t invalid(value);
        });
        test::expect_throws<std::invalid_argument>([&] {
            auto value = known_unique_name;
            value.back() = '-';
            [[maybe_unused]] const api::module_name_t invalid(value);
        });
        test::expect_throws<std::invalid_argument>([] {
            [[maybe_unused]] const auto invalid = api::module_name_t::from_friendly_name("");
        });
        test::expect_throws<std::invalid_argument>([] {
            [[maybe_unused]] const auto invalid = api::module_name_t::from_friendly_name("not-valid");
        });

        const api::workspace_name_t ws0("ws0");
        const api::workspace_name_t ws2("ws2");
        const api::workspace_name_t ws10("ws10");
        test::expect(std::equal_to<>(), ws0.relative_path(), filesystem_api::relative_path_t("ws0"));
        test::expect(std::equal_to<>(), ws0.order_position(), std::uint32_t(0));
        test::expect(std::equal_to<>(), ws10.order_position(), std::uint32_t(10));
        test::expect(std::identity(), ws0 == api::workspace_name_t("ws0"));
        test::expect(std::identity(), ws0 < ws2);
        test::expect(std::identity(), ws2 < ws10);
        test::expect(std::identity(), ws0 <= ws0);
        test::expect(std::identity(), ws0 <= ws10);
        test::expect(std::equal_to<>(), std::format("{}", ws10), std::string("ws10"));
        test::expect(std::equal_to<>(), api::workspace_name_hash_t()(ws2),
            api::workspace_name_hash_t()(api::workspace_name_t("ws2"))
        );

        test::expect_throws<std::runtime_error>([] {
            [[maybe_unused]] const api::workspace_name_t invalid("workspace0");
        });
        test::expect_throws<std::invalid_argument>([] {
            [[maybe_unused]] const api::workspace_name_t invalid("ws");
        });
        test::expect_throws<std::invalid_argument>([] {
            [[maybe_unused]] const api::workspace_name_t invalid("wsx");
        });
        test::expect_throws<std::invalid_argument>([] {
            [[maybe_unused]] const api::workspace_name_t invalid("ws4294967296");
        });

        temporary_directory_t temporary_directory;
        const auto root_native = temporary_directory.path() / "root";
        const auto artifacts_native = temporary_directory.path() / "artifacts";
        std::filesystem::create_directories(root_native / "ws0" / known_name.unique_name());
        std::filesystem::create_directories(root_native / "ws0" / later_name.unique_name());

        const api::module_name_t third_name("m03gn7qllwpi68ovctow4jrccj_lexer");
        std::filesystem::create_directories(root_native / "ws2" / third_name.unique_name());
        std::filesystem::create_directories(root_native / "not-a-workspace");

        create_file(root_native / "ws0" / known_name.unique_name() / "api.cpp", "one");
        create_file(root_native / "ws0" / later_name.unique_name() / "api.h", "two");
        create_file(root_native / "ws2" / third_name.unique_name() / "lexer.cpp", "three");

        const auto version_directory_native = temporary_directory.path() / "version-directory";
        const auto version_nested_native = version_directory_native / "nested";
        std::filesystem::create_directories(version_nested_native);
        create_file(version_directory_native / "old", "old");
        create_file(version_nested_native / "latest", "latest");

        const auto now = std::filesystem::file_time_type::clock::now();
        const auto oldest = now - std::chrono::hours(3);
        const auto middle = now - std::chrono::hours(2);
        const auto latest = now - std::chrono::hours(1);
        std::filesystem::last_write_time(version_directory_native, oldest);
        std::filesystem::last_write_time(version_nested_native, middle);
        std::filesystem::last_write_time(version_directory_native / "old", middle);
        std::filesystem::last_write_time(version_nested_native / "latest", latest);

        const filesystem_api::path_t version_directory(version_directory_native);
        test::expect(std::equal_to<>(), api::version_t(version_directory).value,
            api::version_t(latest).value
        );

        const filesystem_api::path_t root(root_native);
        const filesystem_api::path_t artifacts(artifacts_native);
        api::workspace_graph_t graph(root, artifacts);

        test::expect(std::equal_to<>(), graph.root(), root);
        test::expect(std::equal_to<>(), graph.artifact_root(), artifacts);
        test::expect(std::identity(), graph.modules().empty());

        const auto workspaces = graph.workspaces();
        test::expect(std::equal_to<>(), workspaces.size(), std::size_t(2));
        test::expect(std::equal_to<>(), workspaces[0]->name(), ws0);
        test::expect(std::equal_to<>(), workspaces[1]->name(), ws2);
        test::expect(std::identity(), &workspaces[0]->graph() == &graph);
        test::expect(std::equal_to<>(), std::format("{}", *workspaces[0]), std::string("ws0"));

        const auto indexed_names = graph.module_names();
        test::expect(std::equal_to<>(), indexed_names.size(), std::size_t(3));
        test::expect(std::identity(), indexed_names.contains(known_name));
        test::expect(std::identity(), indexed_names.contains(later_name));
        test::expect(std::identity(), indexed_names.contains(third_name));

        auto* first_module = graph.discover_module(known_name);
        test::expect(std::identity(), first_module == graph.discover_module(known_name));
        test::expect(std::identity(), &first_module->workspace() == workspaces[0]);
        test::expect(std::equal_to<>(), first_module->name(), known_name);
        test::expect(std::equal_to<>(), first_module->source_dir(), filesystem_api::path_t(root_native / "ws0" / known_name.unique_name()));
        test::expect(std::equal_to<>(), first_module->artifact_base_dir(), filesystem_api::path_t(artifacts_native / known_name.unique_name()));
        test::expect(std::equal_to<>(), first_module->artifact_latest_dir(), filesystem_api::path_t(artifacts_native / known_name.unique_name() / "latest"));
        test::expect(std::equal_to<>(), std::format("{}", *first_module), known_name.unique_name());
        test::expect(std::identity(), workspaces[0]->find_module(known_name) == first_module);
        test::expect(std::identity(), workspaces[0]->find_module(third_name) == nullptr);

        const auto discovered_version = first_module->version();
        test::expect(std::identity(), 0 < discovered_version.value);
        first_module->version(api::version_t(123));
        test::expect(std::equal_to<>(), first_module->version().value, std::uint64_t(123));

        auto* third_module = graph.discover_module(third_name);
        test::expect(std::identity(), &third_module->workspace() == workspaces[1]);
        const auto discovered_modules = graph.modules();
        test::expect(std::equal_to<>(), discovered_modules.size(), std::size_t(2));
        test::expect(std::identity(), discovered_modules[0] == first_module);
        test::expect(std::identity(), discovered_modules[1] == third_module);

        test::expect_throws<std::runtime_error>([&] {
            const auto missing = api::module_name_t::from_friendly_name("missing");
            [[maybe_unused]] auto* module = graph.discover_module(missing);
        });

        api::workspace_t manual_workspace(graph, api::workspace_name_t("ws9"));
        api::workspace_t later_workspace(graph, api::workspace_name_t("ws10"));
        test::expect(std::identity(), manual_workspace == api::workspace_t(graph, api::workspace_name_t("ws9")));
        test::expect(std::identity(), manual_workspace < later_workspace);
        test::expect(std::identity(), manual_workspace <= manual_workspace);
        test::expect(std::identity(), &manual_workspace.graph() == &graph);
        test::expect(std::equal_to<>(), manual_workspace.name(), api::workspace_name_t("ws9"));
        test::expect(std::equal_to<>(), std::format("{}", manual_workspace), std::string("ws9"));

        const auto manual_name_a = api::module_name_t::from_friendly_name("manual_a");
        const auto manual_name_b = api::module_name_t::from_friendly_name("manual_b");
        auto* manual_module_b = manual_workspace.add_module(std::make_unique<api::module_t>(
            &manual_workspace,
            manual_name_b,
            api::version_t(2)
        ));
        auto* manual_module_a = manual_workspace.add_module(std::make_unique<api::module_t>(
            &manual_workspace,
            manual_name_a,
            api::version_t(1)
        ));
        test::expect_throws<std::invalid_argument>([&] {
            [[maybe_unused]] auto* invalid = manual_workspace.add_module(nullptr);
        });
        test::expect_throws<std::invalid_argument>([&] {
            [[maybe_unused]] const api::module_t invalid(nullptr, manual_name_a, api::version_t(1));
        });
        test::expect_throws<std::invalid_argument>([&] {
            [[maybe_unused]] auto* invalid = manual_workspace.add_module(std::make_unique<api::module_t>(
                &later_workspace,
                api::module_name_t::from_friendly_name("foreign"),
                api::version_t(1)
            ));
        });
        test::expect_throws<std::invalid_argument>([&] {
            [[maybe_unused]] auto* invalid = manual_workspace.add_module(std::make_unique<api::module_t>(
                &manual_workspace,
                manual_name_a,
                api::version_t(3)
            ));
        });
        test::expect(std::identity(), manual_workspace.find_module(manual_name_a) == manual_module_a);
        test::expect(std::identity(), manual_workspace.find_module(manual_name_b) == manual_module_b);
        test::expect(std::identity(), manual_workspace.find_module(known_name) == nullptr);

        const auto manual_modules = manual_workspace.modules();
        test::expect(std::equal_to<>(), manual_modules.size(), std::size_t(2));
        test::expect(std::identity(), manual_modules[0]->name() < manual_modules[1]->name());
        test::expect(std::identity(), &manual_module_a->workspace() == &manual_workspace);
        test::expect(std::equal_to<>(), manual_module_a->name(), manual_name_a);
        test::expect(std::equal_to<>(), manual_module_a->version().value, std::uint64_t(1));
        manual_module_a->version(api::version_t(9));
        test::expect(std::equal_to<>(), manual_module_a->version().value, std::uint64_t(9));
        test::expect(std::equal_to<>(), manual_module_a->source_dir(),
            root / api::workspace_name_t("ws9").relative_path() / filesystem_api::relative_path_t(manual_name_a.unique_name())
        );
        test::expect(std::equal_to<>(), manual_module_a->artifact_base_dir(),
            artifacts / filesystem_api::relative_path_t(manual_name_a.unique_name())
        );
        test::expect(std::equal_to<>(), manual_module_a->artifact_latest_dir(),
            artifacts / filesystem_api::relative_path_t(manual_name_a.unique_name()) / filesystem_api::relative_path_t("latest")
        );

        {
            temporary_directory_t duplicate_root;
            std::filesystem::create_directories(duplicate_root.path() / "ws0" / known_name.unique_name());
            std::filesystem::create_directories(duplicate_root.path() / "ws1" / known_name.unique_name());
            test::expect_throws<std::runtime_error>([&] {
                [[maybe_unused]] const api::workspace_graph_t duplicate_graph(
                    filesystem_api::path_t(duplicate_root.path()),
                    filesystem_api::path_t(duplicate_root.path() / "artifacts")
                );
            });
        }

        {
            temporary_directory_t invalid_module_root;
            std::filesystem::create_directories(invalid_module_root.path() / "ws0" / "not-a-module");
            test::expect_throws<std::invalid_argument>([&] {
                [[maybe_unused]] const api::workspace_graph_t invalid_graph(
                    filesystem_api::path_t(invalid_module_root.path()),
                    filesystem_api::path_t(invalid_module_root.path() / "artifacts")
                );
            });
        }

        environment_guard_t workspace_root_guard("BUILDER_WORKSPACE_ROOT");
        environment_guard_t artifact_root_guard("BUILDER_ARTIFACT_ROOT");
        current_path_guard_t current_path_guard;

        const auto explicit_artifacts_native = temporary_directory.path() / "explicit-artifacts";
        setenv("BUILDER_WORKSPACE_ROOT", root_native.c_str(), 1);
        setenv("BUILDER_ARTIFACT_ROOT", explicit_artifacts_native.c_str(), 1);
        const auto explicit_context = api::invocation_context();
        test::expect(std::equal_to<>(), explicit_context.workspace_root, root);
        test::expect(std::equal_to<>(), explicit_context.artifact_root, filesystem_api::path_t(explicit_artifacts_native));
        test::expect(std::equal_to<>(), std::string(std::getenv("BUILDER_WORKSPACE_ROOT")), root.string());
        test::expect(std::equal_to<>(), std::string(std::getenv("BUILDER_ARTIFACT_ROOT")),
            filesystem_api::path_t(explicit_artifacts_native).string()
        );

        unsetenv("BUILDER_WORKSPACE_ROOT");
        unsetenv("BUILDER_ARTIFACT_ROOT");
        std::filesystem::current_path(root_native);
        const auto default_context = api::invocation_context();
        test::expect(std::equal_to<>(), default_context.workspace_root, root);
        test::expect(std::equal_to<>(), default_context.artifact_root, root / filesystem_api::relative_path_t("artifacts"));

        setenv("BUILDER_WORKSPACE_ROOT", "", 1);
        test::expect_throws<std::runtime_error>([] {
            [[maybe_unused]] const auto context = api::invocation_context();
        });

        setenv("BUILDER_WORKSPACE_ROOT", root_native.c_str(), 1);
        setenv("BUILDER_ARTIFACT_ROOT", "", 1);
        test::expect_throws<std::runtime_error>([] {
            [[maybe_unused]] const auto context = api::invocation_context();
        });

        const api::invocation_context_t aggregate_context {
            .workspace_root = root,
            .artifact_root = artifacts
        };
        test::expect(std::equal_to<>(), aggregate_context.workspace_root, root);
        test::expect(std::equal_to<>(), aggregate_context.artifact_root, artifacts);
    });
}
