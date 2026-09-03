#include <m03gn97n4iusbtl7uthb01wu9m_test_framework/test_framework.h>
#include <m03gn8rf3peew0re4l1s2vvaw6_bootstrap_seed/bootstrap_seed.h>

#include <functional>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace api = m03gn8rf3peew0re4l1s2vvaw6_bootstrap_seed;
namespace filesystem_api = m03gagbhsnusi43zogoacgj2ez_filesystem;
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
            "builder-bootstrap-seed-public-api-{}-{}",
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

void touch_native(const std::filesystem::path& path) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("failed to create bootstrap-seed fixture");
    }
}

} // namespace

int main() {
    return test::run([] {
        temporary_directory_t temporary_directory;
        const auto workspace_root_native = temporary_directory.path() / "workspace";
        const auto artifact_root_native = temporary_directory.path() / "artifacts";

        for (const auto& module_name : bootstrap_module_names) {
            const auto module_dir = workspace_root_native / "ws0" / module_name;
            std::filesystem::create_directories(module_dir);
            touch_native(module_dir / "source.cpp");
        }

        graph_api::workspace_graph_t graph {
            filesystem_api::path_t(workspace_root_native),
            filesystem_api::path_t(artifact_root_native)
        };

        auto& seed_module = api::module(graph);
        test::expect(std::equal_to<>(), seed_module.name().unique_name(),
            std::string("m03gagbhst621faiop1rztfkqp_builder_cli")
        );
        test::expect(std::identity(), &seed_module == &api::module(graph));
        test::expect(std::identity(), api::is_module(seed_module));

        const auto modules = api::modules(graph);
        test::expect(std::equal_to<>(), modules.size(), bootstrap_module_names.size());
        for (std::size_t i = 0; i < modules.size(); ++i) {
            test::expect(std::equal_to<>(), modules[i]->name().unique_name(), bootstrap_module_names[i]);
            test::expect(std::identity(), api::is_module(*modules[i]));
            modules[i]->version(graph_api::version_t(static_cast<std::uint64_t>(i + 1)));
        }
        modules[5]->version(graph_api::version_t(900));
        test::expect(std::equal_to<>(), api::version(graph).value, std::uint64_t(900));

        graph_api::workspace_t ws0(graph, graph_api::workspace_name_t("ws0"));
        graph_api::workspace_t ws1(graph, graph_api::workspace_name_t("ws1"));
        const auto unknown_name = graph_api::module_name_t::from_friendly_name("not_seeded");
        const graph_api::module_t unknown_module(
            &ws0,
            unknown_name,
            graph_api::version_t(1)
        );
        test::expect(std::identity(), !api::is_module(unknown_module));

        const graph_api::module_t misplaced_known_module(
            &ws1,
            graph_api::module_name_t(bootstrap_module_names.front()),
            graph_api::version_t(1)
        );
        test::expect(std::identity(), !api::is_module(misplaced_known_module));

        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto path = api::builder_plugin_path(graph);
        });

        const auto expected_plugin = seed_module.artifact_latest_dir()
            / filesystem_api::relative_path_t("builder")
            / filesystem_api::relative_path_t("install")
            / filesystem_api::relative_path_t("builder.so");
        touch_native(expected_plugin.to_native_path());
        test::expect(std::equal_to<>(), api::builder_plugin_path(graph), expected_plugin);

        {
            temporary_directory_t missing_seed_directory;
            const auto missing_root = missing_seed_directory.path() / "workspace";
            std::filesystem::create_directories(missing_root / "ws0");
            graph_api::workspace_graph_t missing_seed_graph(
                filesystem_api::path_t(missing_root),
                filesystem_api::path_t(missing_seed_directory.path() / "artifacts")
            );
            test::expect_throws<std::runtime_error>([&] {
                [[maybe_unused]] auto& missing = api::module(missing_seed_graph);
            });
        }

        {
            temporary_directory_t misplaced_directory;
            const auto misplaced_root = misplaced_directory.path() / "workspace";
            for (std::size_t i = 0; i < bootstrap_module_names.size(); ++i) {
                const char* workspace = i == 0 ? "ws1" : "ws0";
                std::filesystem::create_directories(
                    misplaced_root / workspace / bootstrap_module_names[i]
                );
            }
            graph_api::workspace_graph_t misplaced_graph(
                filesystem_api::path_t(misplaced_root),
                filesystem_api::path_t(misplaced_directory.path() / "artifacts")
            );
            test::expect_throws<std::runtime_error>([&] {
                [[maybe_unused]] const auto values = api::modules(misplaced_graph);
            });
        }
    });
}
