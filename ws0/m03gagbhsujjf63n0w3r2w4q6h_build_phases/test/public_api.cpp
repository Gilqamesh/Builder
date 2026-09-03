#include <m03gn97n4iusbtl7uthb01wu9m_test_framework/test_framework.h>
#include <m03gagbhsujjf63n0w3r2w4q6h_build_phases/build_phases.h>

#include <functional>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

namespace api = m03gagbhsujjf63n0w3r2w4q6h_build_phases;
namespace filesystem_api = m03gagbhsnusi43zogoacgj2ez_filesystem;
namespace graph_api = m03gagbhsp2drqq3gkop8pzfrm_workspace_graph;
namespace test = m03gn97n4iusbtl7uthb01wu9m_test_framework;

namespace toolchain_api = m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain;

namespace {

class temporary_directory_t {
public:
    temporary_directory_t() {
        m_path = std::filesystem::temp_directory_path() / std::format(
            "builder-build-phases-public-api-{}-{}",
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
        throw std::runtime_error("failed to create build-phases fixture");
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
        throw std::runtime_error("failed to build empty builder plugin");
    }
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

bool contains_name(
    const std::vector<graph_api::module_name_t>& names,
    const graph_api::module_name_t& name
) {
    for (const auto& candidate : names) {
        if (candidate == name) {
            return true;
        }
    }
    return false;
}

} // namespace

int main() {
    return test::run([] {
        const api::discovered_module_dependencies_t empty_dependencies;
        test::expect(std::identity(), empty_dependencies.module_dependencies.empty());
        test::expect(std::identity(), empty_dependencies.builder_dependencies.empty());

        temporary_directory_t temporary_directory;
        const auto workspace_root_native = temporary_directory.path() / "workspace";
        const auto artifact_root_native = temporary_directory.path() / "artifacts";

        const graph_api::module_name_t builder_cli_name(
            "m03gagbhst621faiop1rztfkqp_builder_cli"
        );
        const graph_api::module_name_t subject_name(
            "m03gagbhsujjf63n0w3r2w4q6h_build_phases"
        );
        const graph_api::module_name_t discovery_owner_name(
            "m03gn8rf3pe8dkpk1uwsemhhmd_artifact_store"
        );
        const graph_api::module_name_t module_dependency_name(
            "m03gagbhsnusi43zogoacgj2ez_filesystem"
        );
        const graph_api::module_name_t builder_dependency_name(
            "m03gagbhsvr0m5w15urj0o291m_process"
        );

        const auto builder_cli_dir = workspace_root_native / "ws0" / builder_cli_name.unique_name();
        const auto subject_dir = workspace_root_native / "ws0" / subject_name.unique_name();
        const auto discovery_owner_dir = workspace_root_native / "ws0" / discovery_owner_name.unique_name();
        const auto module_dependency_dir = workspace_root_native / "ws0" / module_dependency_name.unique_name();
        const auto builder_dependency_dir = workspace_root_native / "ws0" / builder_dependency_name.unique_name();

        std::filesystem::create_directories(builder_cli_dir);
        std::filesystem::create_directories(subject_dir);
        std::filesystem::create_directories(discovery_owner_dir);
        std::filesystem::create_directories(module_dependency_dir);
        std::filesystem::create_directories(builder_dependency_dir);

        write_file(subject_dir / "api.h", "#pragma once\nextern \"C\" int phase_value();\n");
        write_file(subject_dir / "extra.hpp", "#pragma once\ninline int extra_value() { return 1; }\n");
        write_file(subject_dir / "api.cpp", "#include \"api.h\"\nextern \"C\" int phase_value() { return 42; }\n");
        write_file(
            subject_dir / "cli.cpp",
            std::format(
                "#include <{}/api.h>\nint main() {{ return phase_value() == 42 ? 0 : 1; }}\n",
                subject_name.unique_name()
            )
        );
        write_file(subject_dir / "builder.cpp", "int subject_builder_source;\n");
        write_file(subject_dir / "manual.inc", "manual\n");
        write_file(subject_dir / "data.txt", "data\n");
        write_file(subject_dir / "built.txt", "built\n");

        write_file(
            discovery_owner_dir / "api.cpp",
            std::format("#include <{}/api.h>\n", module_dependency_name.unique_name())
        );
        write_file(
            discovery_owner_dir / "builder.cpp",
            std::format("#include <{}/api.h>\n", builder_dependency_name.unique_name())
        );
        write_file(module_dependency_dir / "api.h", "#pragma once\n");
        write_file(builder_dependency_dir / "api.h", "#pragma once\n");
        write_file(builder_cli_dir / "api.cpp", "int builder_cli_source;\n");

        const auto plugin_native = artifact_root_native
            / builder_cli_name.unique_name()
            / "latest"
            / "builder"
            / "install"
            / "builder.so";
        std::filesystem::create_directories(plugin_native.parent_path());
        build_empty_plugin(plugin_native);

        graph_api::workspace_graph_t graph {
            filesystem_api::path_t(workspace_root_native),
            filesystem_api::path_t(artifact_root_native)
        };
        auto* subject = graph.discover_module(subject_name);
        auto* discovery_owner = graph.discover_module(discovery_owner_name);
        test::expect(std::identity(), subject != nullptr);
        test::expect(std::identity(), discovery_owner != nullptr);

        const filesystem_api::path_t arbitrary_root(temporary_directory.path() / "arbitrary-root");
        std::filesystem::create_directories(arbitrary_root.to_native_path());
        const api::source_phase_t::installed_t source_installed(arbitrary_root);
        const api::interface_phase_t::installed_t interface_installed(arbitrary_root);
        const api::library_phase_t::installed_t library_installed(arbitrary_root);
        test::expect(std::equal_to<>(), source_installed.root(), arbitrary_root);
        test::expect(std::equal_to<>(), interface_installed.root(), arbitrary_root);
        test::expect(std::equal_to<>(), library_installed.root(), arbitrary_root);

        {
            api::source_phase_t source_phase(*subject, nullptr);
            test::expect(std::equal_to<>(), source_phase.name(), std::string_view("source"));
            test::expect(std::equal_to<>(), source_phase.source_dir(), subject->source_dir());
            test::expect_throws<std::runtime_error>([&] {
                [[maybe_unused]] const auto path = source_phase.build_dir();
            });
            test::expect_throws<std::runtime_error>([&] {
                [[maybe_unused]] const auto binary = source_phase.install<api::binary_phase_t>();
            });

            const auto installed = source_phase.install<api::source_phase_t>();
            test::expect(std::identity(), filesystem_api::is_regular_file(
                installed.root() / filesystem_api::relative_path_t("api.cpp")
            ));
            test::expect(std::identity(), filesystem_api::is_regular_file(
                installed.root() / filesystem_api::relative_path_t("data.txt")
            ));
            test::expect(std::identity(), filesystem_api::is_directory(source_phase.build_dir()));

            const auto generated = source_phase.build_dir()
                / filesystem_api::relative_path_t("generated.txt");
            write_file(generated.to_native_path(), "generated\n");
            source_phase.install_source(generated);
            test::expect(std::identity(), filesystem_api::is_regular_file(
                installed.root() / filesystem_api::relative_path_t("generated.txt")
            ));

            const auto direct = source_phase.build_dir()
                / filesystem_api::relative_path_t("direct.txt");
            write_file(direct.to_native_path(), "direct\n");
            source_phase.install(direct);
            test::expect(std::identity(), filesystem_api::is_regular_file(
                installed.root() / filesystem_api::relative_path_t("direct.txt")
            ));

            const filesystem_api::rooted_path_t rooted_direct(
                source_phase.build_dir(),
                filesystem_api::relative_path_t("direct.txt")
            );
            test::expect_throws<std::runtime_error>([&] {
                source_phase.install(rooted_direct);
            });

            const auto external = filesystem_api::path_t(temporary_directory.path() / "external.txt");
            write_file(external.to_native_path(), "external\n");
            const auto built_external = source_phase.build(
                external,
                filesystem_api::relative_path_t("external.alias")
            );
            test::expect(std::equal_to<>(), built_external.rooted_path().root(), source_phase.build_dir());
            test::expect(std::equal_to<>(), built_external.rooted_path().relative_path(),
                filesystem_api::relative_path_t("external.alias")
            );
            source_phase.install(built_external);
            test::expect(std::identity(), filesystem_api::exists(
                installed.root() / filesystem_api::relative_path_t("external.alias")
            ));

            test::expect_throws<std::runtime_error>([&] {
                [[maybe_unused]] const auto duplicate = source_phase.build(
                    external,
                    filesystem_api::relative_path_t("external.alias")
                );
            });
            test::expect_throws<std::runtime_error>([&] {
                [[maybe_unused]] const auto missing_built = source_phase.build(
                    filesystem_api::path_t(temporary_directory.path() / "missing-external"),
                    filesystem_api::relative_path_t("missing.alias")
                );
            });
            test::expect_throws<std::runtime_error>([&] {
                [[maybe_unused]] const auto invalid = source_phase.build(external);
            });
            test::expect_throws<std::runtime_error>([&] {
                [[maybe_unused]] const auto invalid = source_phase.build(
                    filesystem_api::rooted_path_t(installed.root(), filesystem_api::relative_path_t("api.cpp"))
                );
            });
            test::expect_throws<std::runtime_error>([&] {
                [[maybe_unused]] const auto invalid = source_phase.source("api.cpp");
            });
            test::expect_throws<std::runtime_error>([&] {
                source_phase.install_source_tree();
            });
        }

        {
            std::unique_ptr<api::phase_base_t> previous =
                std::make_unique<api::source_phase_t>(*subject, nullptr);
            api::interface_phase_t interface_phase(*subject, std::move(previous));
            test::expect(std::equal_to<>(), interface_phase.name(), std::string_view("interface"));

            const auto installed = interface_phase.install<api::interface_phase_t>();
            const auto module_root = installed.root()
                / filesystem_api::relative_path_t(subject_name.unique_name());
            test::expect(std::identity(), filesystem_api::is_regular_file(
                module_root / filesystem_api::relative_path_t("api.h")
            ));
            test::expect(std::identity(), filesystem_api::is_regular_file(
                module_root / filesystem_api::relative_path_t("extra.hpp")
            ));
            test::expect(std::identity(), !filesystem_api::exists(
                module_root / filesystem_api::relative_path_t("api.cpp")
            ));

            const auto source_output = interface_phase.install<api::source_phase_t>();
            const auto data_path = source_output.root()
                / filesystem_api::relative_path_t("data.txt");
            const auto built_path = interface_phase.build(data_path);
            test::expect(std::equal_to<>(), built_path.rooted_path().root(), source_output.root());
            test::expect(std::equal_to<>(), built_path.rooted_path().relative_path(),
                filesystem_api::relative_path_t("data.txt")
            );
            interface_phase.install(built_path);
            test::expect(std::identity(), filesystem_api::is_regular_file(
                installed.root() / filesystem_api::relative_path_t("data.txt")
            ));

            const filesystem_api::rooted_path_t manual_rooted(
                source_output.root(),
                filesystem_api::relative_path_t("manual.inc")
            );
            const auto built_rooted = interface_phase.build(manual_rooted);
            test::expect(std::equal_to<>(), built_rooted.rooted_path().path(), manual_rooted.path());
            interface_phase.install_interface(manual_rooted);
            test::expect(std::identity(), filesystem_api::is_regular_file(
                module_root / filesystem_api::relative_path_t("manual.inc")
            ));

            const auto selected_source = interface_phase.source("built.txt");
            test::expect(std::equal_to<>(), selected_source.rooted_path().relative_path(),
                filesystem_api::relative_path_t("built.txt")
            );
            interface_phase.install(selected_source.rooted_path());
            test::expect(std::identity(), filesystem_api::is_regular_file(
                installed.root() / filesystem_api::relative_path_t("built.txt")
            ));

            const auto staged = interface_phase.build_interface_as(
                source_output.root() / filesystem_api::relative_path_t("manual.inc"),
                filesystem_api::relative_path_t("generated/generated.hpp")
            );
            test::expect(std::identity(), filesystem_api::is_regular_file(staged));
            interface_phase.install_interface(staged);
            test::expect(std::identity(), filesystem_api::is_regular_file(
                module_root / filesystem_api::relative_path_t("generated/generated.hpp")
            ));

            const auto raw = interface_phase.build_dir()
                / filesystem_api::relative_path_t("raw.bin");
            write_file(raw.to_native_path(), "raw\n");
            interface_phase.install(raw);
            test::expect(std::identity(), filesystem_api::is_regular_file(
                installed.root() / filesystem_api::relative_path_t("raw.bin")
            ));

            const auto external = filesystem_api::path_t(temporary_directory.path() / "interface-external");
            write_file(external.to_native_path(), "external\n");
            const auto external_built = interface_phase.build(
                external,
                filesystem_api::relative_path_t("interface-external.alias")
            );
            interface_phase.install(external_built);
            test::expect(std::identity(), filesystem_api::exists(
                installed.root() / filesystem_api::relative_path_t("interface-external.alias")
            ));

            test::expect_throws<std::runtime_error>([&] {
                [[maybe_unused]] const auto invalid = interface_phase.build(
                    filesystem_api::path_t(temporary_directory.path())
                );
            });
            test::expect_throws<std::runtime_error>([&] {
                const filesystem_api::rooted_path_t wrong_root(
                    subject->source_dir(),
                    filesystem_api::relative_path_t("api.cpp")
                );
                [[maybe_unused]] const auto invalid = interface_phase.build(wrong_root);
            });
            test::expect_throws<std::runtime_error>([&] {
                interface_phase.install(
                    filesystem_api::path_t(temporary_directory.path() / "outside-install")
                );
            });
            test::expect_throws<std::runtime_error>([&] {
                interface_phase.install_headers_from_source();
            });
        }

        {
            std::unique_ptr<api::phase_base_t> phase;
            phase = std::make_unique<api::source_phase_t>(*subject, std::move(phase));
            phase = std::make_unique<api::interface_phase_t>(*subject, std::move(phase));
            api::library_phase_t library_phase(*subject, std::move(phase));
            test::expect(std::equal_to<>(), library_phase.name(), std::string_view("library"));
            test::expect(std::identity(), library_phase.validations().empty());

            const auto source_file = library_phase.source("api.cpp");
            const std::vector<toolchain_api::define_t> defines {
                toolchain_api::define_t("VALIDATION_DEFINE", "value")
            };
            library_phase.validate_library(
                "validation",
                { source_file },
                defines,
                { "first", "second" }
            );
            test::expect(std::equal_to<>(), library_phase.validations().size(), std::size_t(1));
            const auto& validation = library_phase.validations().front();
            test::expect(std::equal_to<>(), validation.name, std::string("validation"));
            test::expect(std::equal_to<>(), validation.source_files.size(), std::size_t(1));
            test::expect(std::equal_to<>(), validation.source_files[0].rooted_path().relative_path(),
                filesystem_api::relative_path_t("api.cpp")
            );
            test::expect(std::equal_to<>(), validation.defines.size(), std::size_t(1));
            test::expect(std::equal_to<>(), validation.defines[0].key(), std::string("VALIDATION_DEFINE"));
            test::expect(std::identity(), validation.arguments == std::vector<std::string>({ "first", "second" }));

            test::expect_throws<std::runtime_error>([&] {
                library_phase.validate_library("", { source_file });
            });
            test::expect_throws<std::runtime_error>([&] {
                library_phase.validate_library("bad/name", { source_file });
            });
            test::expect_throws<std::runtime_error>([&] {
                library_phase.validate_library("empty", {});
            });

            const auto installed = library_phase.install<api::library_phase_t>();
            const auto library_path = installed.root()
                / filesystem_api::relative_path_t(std::format("lib{}.so", subject_name));
            test::expect(std::identity(), filesystem_api::is_regular_file(library_path));
            test::expect(std::equal_to<>(), library_phase.install<api::library_phase_t>().root(), installed.root());

            test::expect_throws<std::runtime_error>([&] {
                [[maybe_unused]] const auto value = library_phase.build_library({ source_file }, {});
            });
            test::expect_throws<std::runtime_error>([&] {
                library_phase.install_library(library_path);
            });
            test::expect_throws<std::runtime_error>([&] {
                library_phase.install_library(source_file);
            });

            const api::library_phase_t::validation_t aggregate_validation {
                .name = "aggregate",
                .source_files = { source_file },
                .defines = {},
                .arguments = { "argument" }
            };
            test::expect(std::equal_to<>(), aggregate_validation.name, std::string("aggregate"));
            test::expect(std::equal_to<>(), aggregate_validation.source_files.size(), std::size_t(1));
            test::expect(std::equal_to<>(), aggregate_validation.arguments.size(), std::size_t(1));
        }

        {
            const auto fake_binary_root = filesystem_api::path_t(
                temporary_directory.path() / "fake-binary-root"
            );
            const auto fake_binary = fake_binary_root
                / filesystem_api::relative_path_t("tool")
                / filesystem_api::relative_path_t("install")
                / filesystem_api::relative_path_t("tool");
            write_file(fake_binary.to_native_path(), "binary\n");
            const api::binary_phase_t::installed_t installed(fake_binary_root);
            test::expect(std::equal_to<>(), installed.root(), fake_binary_root);
            test::expect(std::equal_to<>(), installed.target("tool"), fake_binary);
            test::expect_throws<std::runtime_error>([&] {
                [[maybe_unused]] const auto target = installed.target("");
            });
            test::expect_throws<std::runtime_error>([&] {
                [[maybe_unused]] const auto target = installed.target("bad/name");
            });
            test::expect_throws<std::runtime_error>([&] {
                [[maybe_unused]] const auto target = installed.target("missing");
            });
        }

        {
            std::unique_ptr<api::phase_base_t> phase;
            phase = std::make_unique<api::source_phase_t>(*subject, std::move(phase));
            phase = std::make_unique<api::interface_phase_t>(*subject, std::move(phase));
            phase = std::make_unique<api::library_phase_t>(*subject, std::move(phase));
            api::binary_phase_t binary_phase(*subject, "", std::move(phase));
            test::expect(std::equal_to<>(), binary_phase.name(), std::string_view("binary"));
            test::expect(std::identity(), binary_phase.should_install_target("cli"));
            test::expect(std::identity(), binary_phase.should_install_target("anything"));

            const auto installed = binary_phase.install<api::binary_phase_t>();
            const auto cli = installed.target("cli");
            test::expect(std::equal_to<>(), run_binary(cli), 0);

            const auto cli_source = binary_phase.source("cli.cpp");
            binary_phase.install_binary("extra", { cli_source });
            const auto extra = installed.target("extra");
            test::expect(std::equal_to<>(), run_binary(extra), 0);
            const auto data_artifact = binary_phase.source("data.txt");
            binary_phase.install_binary("extra", { cli_source }, {}, { data_artifact });
            binary_phase.install_binary("extra", { cli_source }, {}, { data_artifact });
            test::expect(std::identity(), filesystem_api::is_regular_file(
                installed.root()
                    / filesystem_api::relative_path_t("extra/install/data.txt")
            ));
            binary_phase.install_binary("extra", { cli_source });
            test::expect(std::equal_to<>(), run_binary(installed.target("extra")), 0);

            test::expect_throws<std::runtime_error>([&] {
                binary_phase.install_binary("", { cli_source });
            });
            test::expect_throws<std::runtime_error>([&] {
                binary_phase.install_binary("bad/name", { cli_source });
            });
        }

        {
            std::unique_ptr<api::phase_base_t> phase;
            phase = std::make_unique<api::source_phase_t>(*subject, std::move(phase));
            phase = std::make_unique<api::interface_phase_t>(*subject, std::move(phase));
            phase = std::make_unique<api::library_phase_t>(*subject, std::move(phase));
            api::binary_phase_t selected(*subject, "selected", std::move(phase));
            test::expect(std::identity(), selected.should_install_target("selected"));
            test::expect(std::identity(), !selected.should_install_target("other"));
            test::expect_no_throw([&] {
                selected.install_binary("other/path", {});
            });
        }

        {
            auto phase = api::phase_base_t::make(*subject);
            test::expect(std::equal_to<>(), phase->name(), std::string_view("binary"));
            auto* binary = dynamic_cast<api::binary_phase_t*>(phase.get());
            test::expect(std::identity(), binary != nullptr);
            test::expect(std::identity(), binary->should_install_target("any"));

            auto selected_phase = api::phase_base_t::make(*subject, "chosen");
            auto* selected = dynamic_cast<api::binary_phase_t*>(selected_phase.get());
            test::expect(std::identity(), selected != nullptr);
            test::expect(std::identity(), selected->should_install_target("chosen"));
            test::expect(std::identity(), !selected->should_install_target("other"));
            test::expect(std::equal_to<>(), selected_phase->install<api::source_phase_t>().root(),
                phase->install<api::source_phase_t>().root());
        }

        const auto dependencies = api::discover_module_dependencies(*discovery_owner);
        test::expect(std::equal_to<>(), dependencies.module_dependencies.size(), std::size_t(1));
        test::expect(std::equal_to<>(), dependencies.builder_dependencies.size(), std::size_t(1));
        test::expect(std::identity(), contains_name(dependencies.module_dependencies, module_dependency_name));
        test::expect(std::identity(), contains_name(dependencies.builder_dependencies, builder_dependency_name));
    });
}
