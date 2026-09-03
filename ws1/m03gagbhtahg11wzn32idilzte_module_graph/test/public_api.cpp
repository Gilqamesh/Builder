#include <m03gn97n4iusbtl7uthb01wu9m_test_framework/test_framework.h>
#include <m03gagbhtahg11wzn32idilzte_module_graph/module_graph.h>

#include <functional>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace api = m03gagbhtahg11wzn32idilzte_module_graph;
namespace filesystem_api = m03gagbhsnusi43zogoacgj2ez_filesystem;
namespace graph_api = m03gagbhsp2drqq3gkop8pzfrm_workspace_graph;
namespace test = m03gn97n4iusbtl7uthb01wu9m_test_framework;


namespace {

class temporary_directory_t {
public:
    temporary_directory_t() {
        m_path = std::filesystem::temp_directory_path() / std::format(
            "builder-module-graph-public-api-{}-{}",
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

void write_file(const std::filesystem::path& path, std::string_view contents) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary);
    if (!output) {
        throw std::runtime_error("failed to create module-graph fixture");
    }
    output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
}

std::string read_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("failed to read module-graph output");
    }
    return std::string(
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>()
    );
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
        throw std::runtime_error("failed to build empty builder plugin");
    }
}

std::size_t module_index(
    const std::vector<const graph_api::module_t*>& modules,
    const graph_api::module_t* module
) {
    for (std::size_t i = 0; i < modules.size(); ++i) {
        if (modules[i] == module) {
            return i;
        }
    }
    throw std::runtime_error("module missing from graph module list");
}

} // namespace

int main() {
    return test::run([] {
        temporary_directory_t temporary_directory;
        const auto workspace_root = temporary_directory.path() / "workspace";
        const auto artifact_root = temporary_directory.path() / "artifacts";

        const graph_api::module_name_t builder_cli(
            "m03gagbhst621faiop1rztfkqp_builder_cli"
        );
        const graph_api::module_name_t owner_name(
            "m03gn8rf3pe8dkpk1uwsemhhmd_artifact_store"
        );
        const graph_api::module_name_t module_dependency_name(
            "m03gagbhsnusi43zogoacgj2ez_filesystem"
        );
        const graph_api::module_name_t builder_dependency_name(
            "m03gagbhsvr0m5w15urj0o291m_process"
        );
        const graph_api::module_name_t undiscovered_name(
            "m03ge9ij43jyxy821pda20jhwh_typesystem"
        );

        for (const auto& name : {
            builder_cli,
            owner_name,
            module_dependency_name,
            builder_dependency_name
        }) {
            std::filesystem::create_directories(
                workspace_root / "ws0" / name.unique_name()
            );
        }
        std::filesystem::create_directories(
            workspace_root / "ws1" / undiscovered_name.unique_name()
        );

        write_file(
            workspace_root / "ws0" / owner_name.unique_name() / "api.cpp",
            std::format(
                "#include <{}/api.h>\n",
                module_dependency_name.unique_name()
            )
        );
        write_file(
            workspace_root / "ws0" / owner_name.unique_name() / "builder.cpp",
            std::format(
                "#include <{}/api.h>\n",
                builder_dependency_name.unique_name()
            )
        );
        write_file(
            workspace_root / "ws0" / module_dependency_name.unique_name() / "api.h",
            "#pragma once\n"
        );
        write_file(
            workspace_root / "ws0" / module_dependency_name.unique_name() / "builder.cpp",
            ""
        );
        write_file(
            workspace_root / "ws0" / builder_dependency_name.unique_name() / "api.h",
            "#pragma once\n"
        );
        write_file(
            workspace_root / "ws0" / builder_dependency_name.unique_name() / "builder.cpp",
            ""
        );
        write_file(
            workspace_root / "ws0" / builder_cli.unique_name() / "builder.cpp",
            ""
        );

        const auto plugin = artifact_root
            / builder_cli.unique_name()
            / "latest"
            / "builder"
            / "install"
            / "builder.so";
        std::filesystem::create_directories(plugin.parent_path());
        build_empty_plugin(plugin);

        graph_api::workspace_graph_t graph {
            filesystem_api::path_t(workspace_root),
            filesystem_api::path_t(artifact_root)
        };
        auto* module_dependency = graph.discover_module(module_dependency_name);
        auto* owner = graph.discover_module(owner_name);
        auto* builder_dependency = graph.discover_module(builder_dependency_name);
        const auto modules = graph.modules();

        const auto owner_index = module_index(modules, owner);
        const auto module_dependency_index = module_index(modules, module_dependency);
        const auto builder_dependency_index = module_index(modules, builder_dependency);

        const filesystem_api::path_t dot_output(
            temporary_directory.path() / "graphs" / "modules.dot"
        );
        test::expect(std::equal_to<>(), api::write_dot(graph, *owner, dot_output), dot_output);
        test::expect(std::identity(), filesystem_api::is_regular_file(dot_output));

        const auto dot = read_file(dot_output.to_native_path());
        test::expect(std::identity(), dot.starts_with("digraph BuilderModuleGraph {\n"));
        test::expect(std::identity(), dot.find("subgraph cluster_workspace_0") != std::string::npos);
        test::expect(std::identity(), dot.find("subgraph cluster_workspace_1") != std::string::npos);
        test::expect(std::identity(), dot.find("label=\"ws0\"") != std::string::npos);
        test::expect(std::identity(), dot.find("label=\"ws1\"") != std::string::npos);
        test::expect(std::identity(), dot.find(std::format("builder\\n{}", owner_name.unique_name()))
                != std::string::npos
        );
        test::expect(std::identity(), dot.find(std::format("module\\n{}", owner_name.unique_name()))
                != std::string::npos
        );
        test::expect(std::identity(), dot.find(std::format(
                "p{} -> m{} [label=\"builds\"]",
                owner_index,
                owner_index
            )) != std::string::npos
        );
        test::expect(std::identity(), dot.find(std::format(
                "m{} -> m{} [label=\"module\"]",
                owner_index,
                module_dependency_index
            )) != std::string::npos
        );
        test::expect(std::identity(), dot.find(std::format(
                "p{} -> m{} [label=\"builder\"]",
                owner_index,
                builder_dependency_index
            )) != std::string::npos
        );
        test::expect(std::identity(), dot.find("penwidth=2.2, color=\"#111111\"") != std::string::npos
        );

        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto value = api::write_dot(
                graph,
                *owner,
                filesystem_api::path_t(temporary_directory.path() / "wrong.svg")
            );
        });
        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto value = api::write_dot(graph, *owner, dot_output);
        });

        const filesystem_api::path_t svg_output(
            temporary_directory.path() / "graphs" / "modules.svg"
        );
        const auto temporary_dot = svg_output + "_tmp.dot";
        write_file(temporary_dot.to_native_path(), "stale");
        test::expect(std::equal_to<>(), api::render_svg(graph, *owner, svg_output), svg_output);
        test::expect(std::identity(), filesystem_api::is_regular_file(svg_output));
        test::expect(std::identity(), !filesystem_api::exists(temporary_dot));
        const auto svg = read_file(svg_output.to_native_path());
        test::expect(std::identity(), svg.find("<svg") != std::string::npos);
        test::expect(std::identity(), svg.find(owner_name.unique_name()) != std::string::npos);

        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto value = api::render_svg(
                graph,
                *owner,
                filesystem_api::path_t(temporary_directory.path() / "wrong.png")
            );
        });
        test::expect_throws<std::runtime_error>([&] {
            [[maybe_unused]] const auto value = api::render_svg(graph, *owner, svg_output);
        });
        test::expect(std::identity(), !filesystem_api::exists(temporary_dot));
    });
}
