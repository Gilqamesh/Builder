#include <m03gn97n4iusbtl7uthb01wu9m_test_framework/test_framework.h>
#include <m03gn8rf3pe86v64vphnaam6rl_source_dependencies/source_dependencies.h>

#include <functional>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace api = m03gn8rf3pe86v64vphnaam6rl_source_dependencies;
namespace filesystem_api = m03gagbhsnusi43zogoacgj2ez_filesystem;
namespace graph_api = m03gagbhsp2drqq3gkop8pzfrm_workspace_graph;
namespace test = m03gn97n4iusbtl7uthb01wu9m_test_framework;


namespace {

class temporary_directory_t {
public:
    temporary_directory_t() {
        m_path = std::filesystem::temp_directory_path() / std::format(
            "builder-source-dependencies-public-api-{}-{}",
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

void write_file(const std::filesystem::path& path, std::string_view contents) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary);
    if (!output) {
        throw std::runtime_error("failed to create test file");
    }
    output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
}

std::vector<std::string> relative_strings(
    const std::vector<filesystem_api::rooted_path_t>& paths
) {
    std::vector<std::string> result;
    result.reserve(paths.size());
    for (const auto& path : paths) {
        result.push_back(path.relative_path().string());
    }
    std::sort(result.begin(), result.end());
    return result;
}

std::set<const graph_api::module_t*> pointer_set(
    const std::vector<graph_api::module_t*>& modules
) {
    return std::set<const graph_api::module_t*>(modules.begin(), modules.end());
}

} // namespace

int main() {
    return test::run([] {
        static_assert(api::dependency_mode_t::MODULE != api::dependency_mode_t::BUILDER);

        const api::scan_t empty_scan;
        test::expect(std::identity(), empty_scan.local_files.empty());
        test::expect(std::identity(), empty_scan.dependencies.empty());

        temporary_directory_t temporary_directory;

        const auto library_root_native = temporary_directory.path() / "library-root";
        write_file(library_root_native / "api.cpp", "");
        write_file(library_root_native / "api.h", "");
        write_file(library_root_native / "extra.c", "");
        write_file(library_root_native / "extra.hpp", "");
        write_file(library_root_native / "builder.cpp", "");
        write_file(library_root_native / "cli.cpp", "");
        write_file(library_root_native / "nested" / "implementation.cpp", "");
        write_file(library_root_native / "nested" / "builder.cpp", "");
        write_file(library_root_native / "cli" / "command.cpp", "");
        write_file(library_root_native / "test" / "public_api.cpp", "");
        write_file(library_root_native / "notes.txt", "");

        const auto library_files = relative_strings(api::library_source_files(
            filesystem_api::path_t(library_root_native)
        ));
        test::expect(std::identity(), library_files == std::vector<std::string>({
            "api.cpp",
            "api.h",
            "extra.c",
            "extra.hpp",
            "nested/implementation.cpp"
        }));

        const graph_api::module_name_t dependency_a_name(
            "m03gagbht2l61mj6qitacwbmea_byte_stream"
        );
        const graph_api::module_name_t dependency_c_name(
            "m03gagbhtft23yhjwpp881tfmc_uuid"
        );
        const graph_api::module_name_t dependency_b_name(
            "m03gn7qllwpi68ovctow4jrccj_lexer"
        );
        const graph_api::module_name_t owner_name(
            "m03gn8rf3pe86v64vphnaam6rl_source_dependencies"
        );
        const graph_api::module_name_t same_workspace_name(
            "m03gn8rf3pe8dkpk1uwsemhhmd_artifact_store"
        );
        const graph_api::module_name_t later_name(
            "m03gn8rf3peew0re4l1s2vvaw6_bootstrap_seed"
        );

        const auto workspace_root_native = temporary_directory.path() / "workspace-root";
        const auto artifact_root_native = temporary_directory.path() / "artifacts";

        const auto dependency_a_dir = workspace_root_native / "ws0" / dependency_a_name.unique_name();
        const auto dependency_c_dir = workspace_root_native / "ws0" / dependency_c_name.unique_name();
        const auto dependency_b_dir = workspace_root_native / "ws1" / dependency_b_name.unique_name();
        const auto owner_dir = workspace_root_native / "ws2" / owner_name.unique_name();
        const auto same_workspace_dir = workspace_root_native / "ws2" / same_workspace_name.unique_name();
        const auto later_dir = workspace_root_native / "ws3" / later_name.unique_name();

        std::filesystem::create_directories(dependency_a_dir);
        std::filesystem::create_directories(dependency_c_dir);
        std::filesystem::create_directories(dependency_b_dir);
        std::filesystem::create_directories(owner_dir / "nested");
        std::filesystem::create_directories(same_workspace_dir);
        std::filesystem::create_directories(later_dir);

        const auto unknown_name = graph_api::module_name_t::from_friendly_name("unknown");
        write_file(
            owner_dir / "main.cpp",
            std::format(
                "#include \"local.h\"\n"
                "#include <{}/api.h>\n"
                "#include <{}/api.h>\n"
                "#include <{}/duplicate.h>\n"
                "#include <{}/self.h>\n"
                "#include <{}/unknown.h>\n"
                "#include \"/absolute/path.h\"\n",
                dependency_a_name.unique_name(),
                dependency_b_name.unique_name(),
                dependency_a_name.unique_name(),
                owner_name.unique_name(),
                unknown_name.unique_name()
            )
        );
        write_file(
            owner_dir / "local.h",
            "#include \"nested/nested.hpp\"\n"
        );
        write_file(
            owner_dir / "nested" / "nested.hpp",
            "#include \"local.h\"\n"
        );
        write_file(
            owner_dir / "same-workspace.cpp",
            std::format("#include <{}/api.h>\n", same_workspace_name.unique_name())
        );
        write_file(
            owner_dir / "later.cpp",
            std::format("#include <{}/api.h>\n", later_name.unique_name())
        );

        write_file(
            dependency_a_dir / "api.cpp",
            std::format("#include <{}/api.h>\n", dependency_c_name.unique_name())
        );
        write_file(dependency_c_dir / "api.cpp", "int dependency_c;\n");
        write_file(
            dependency_b_dir / "api.cpp",
            std::format("#include <{}/api.h>\n", dependency_c_name.unique_name())
        );
        write_file(same_workspace_dir / "api.cpp", "int same_workspace;\n");
        write_file(later_dir / "api.cpp", "int later;\n");

        graph_api::workspace_graph_t graph {
            filesystem_api::path_t(workspace_root_native),
            filesystem_api::path_t(artifact_root_native)
        };

        auto* dependency_a = graph.discover_module(dependency_a_name);
        auto* dependency_b = graph.discover_module(dependency_b_name);
        auto* dependency_c = graph.discover_module(dependency_c_name);
        auto* owner = graph.discover_module(owner_name);
        auto* same_workspace = graph.discover_module(same_workspace_name);
        auto* later = graph.discover_module(later_name);

        const filesystem_api::rooted_path_t owner_main(
            owner->source_dir(),
            filesystem_api::relative_path_t("main.cpp")
        );
        const filesystem_api::rooted_path_t owner_same_workspace(
            owner->source_dir(),
            filesystem_api::relative_path_t("same-workspace.cpp")
        );
        const filesystem_api::rooted_path_t owner_later(
            owner->source_dir(),
            filesystem_api::relative_path_t("later.cpp")
        );
        const filesystem_api::rooted_path_t dependency_a_source(
            dependency_a->source_dir(),
            filesystem_api::relative_path_t("api.cpp")
        );
        const filesystem_api::rooted_path_t dependency_b_source(
            dependency_b->source_dir(),
            filesystem_api::relative_path_t("api.cpp")
        );
        const filesystem_api::rooted_path_t dependency_c_source(
            dependency_c->source_dir(),
            filesystem_api::relative_path_t("api.cpp")
        );

        const auto scan = api::scan_sources(
            *owner,
            { owner_main },
            nullptr,
            api::dependency_mode_t::MODULE
        );
        test::expect(std::identity(), relative_strings(scan.local_files) == std::vector<std::string>({ "local.h", "main.cpp", "nested/nested.hpp" })
        );
        test::expect(std::equal_to<>(), scan.dependencies.size(), std::size_t(2));
        test::expect(std::identity(), pointer_set(scan.dependencies) == std::set<const graph_api::module_t*>({ dependency_a, dependency_b })
        );

        const auto excluded_scan = api::scan_sources(
            *owner,
            { owner_main },
            dependency_b,
            api::dependency_mode_t::MODULE
        );
        test::expect(std::equal_to<>(), excluded_scan.dependencies.size(), std::size_t(1));
        test::expect(std::identity(), excluded_scan.dependencies[0] == dependency_a);
        test::expect(std::identity(), relative_strings(excluded_scan.local_files) == relative_strings(scan.local_files));

        const auto builder_scan = api::scan_sources(
            *owner,
            { owner_main },
            nullptr,
            api::dependency_mode_t::BUILDER
        );
        test::expect(std::identity(), pointer_set(builder_scan.dependencies) == std::set<const graph_api::module_t*>({ dependency_a, dependency_b })
        );

        const auto same_workspace_scan = api::scan_sources(
            *owner,
            { owner_same_workspace },
            nullptr,
            api::dependency_mode_t::MODULE
        );
        test::expect(std::equal_to<>(), same_workspace_scan.dependencies.size(), std::size_t(1));
        test::expect(std::identity(), same_workspace_scan.dependencies[0] == same_workspace);

        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto invalid = api::scan_sources(
                *owner,
                { owner_same_workspace },
                nullptr,
                api::dependency_mode_t::BUILDER
            );
        });
        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto invalid = api::scan_sources(
                *owner,
                { owner_later },
                nullptr,
                api::dependency_mode_t::MODULE
            );
        });

        auto deleted_source = owner_main;
        std::filesystem::remove(deleted_source.path().to_native_path());
        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto invalid = api::scan_sources(
                *owner,
                { deleted_source },
                nullptr,
                api::dependency_mode_t::MODULE
            );
        });
        write_file(owner_dir / "main.cpp", "int restored;\n");

        const std::map<const graph_api::module_t*, std::vector<filesystem_api::rooted_path_t>> sources {
            { owner, { filesystem_api::rooted_path_t(owner->source_dir(), filesystem_api::relative_path_t("main.cpp")) } },
            { dependency_a, { dependency_a_source } },
            { dependency_b, { dependency_b_source } },
            { dependency_c, { dependency_c_source } },
            { same_workspace, {} },
            { later, {} }
        };

        const auto source_provider = [&](const graph_api::module_t& module) {
            const auto it = sources.find(&module);
            if (it == sources.end()) {
                return std::vector<filesystem_api::rooted_path_t>();
            }
            return it->second;
        };

        write_file(
            owner_dir / "main.cpp",
            std::format(
                "#include <{}/api.h>\n#include <{}/api.h>\n",
                dependency_a_name.unique_name(),
                dependency_b_name.unique_name()
            )
        );

        const auto dependencies_without_owner = api::dependency_modules(
            *owner,
            sources.at(owner),
            false,
            api::dependency_mode_t::MODULE,
            source_provider
        );
        test::expect(std::equal_to<>(), dependencies_without_owner.size(), std::size_t(3));
        test::expect(std::identity(), pointer_set(dependencies_without_owner) == std::set<const graph_api::module_t*>({ dependency_a, dependency_b, dependency_c })
        );

        const auto dependencies_with_owner = api::dependency_modules(
            *owner,
            sources.at(owner),
            true,
            api::dependency_mode_t::MODULE,
            source_provider
        );
        test::expect(std::equal_to<>(), dependencies_with_owner.size(), std::size_t(4));
        test::expect(std::identity(), dependencies_with_owner.front() == owner);
        test::expect(std::identity(), pointer_set(dependencies_with_owner) == std::set<const graph_api::module_t*>({ owner, dependency_a, dependency_b, dependency_c })
        );

        const auto builder_dependencies = api::dependency_modules(
            *owner,
            sources.at(owner),
            false,
            api::dependency_mode_t::BUILDER,
            source_provider
        );
        test::expect(std::identity(), pointer_set(builder_dependencies) == std::set<const graph_api::module_t*>({ dependency_a, dependency_b, dependency_c })
        );

        const auto dependency_names = api::module_names(dependencies_with_owner);
        test::expect(std::equal_to<>(), dependency_names.size(), dependencies_with_owner.size());
        for (std::size_t i = 0; i < dependency_names.size(); ++i) {
            test::expect(std::equal_to<>(), dependency_names[i], dependencies_with_owner[i]->name());
        }
        test::expect(std::identity(), api::module_names({}).empty());
    });
}
