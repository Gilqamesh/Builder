#include <builder/builder.h>
#include <builder/process/process.h>

#include <iostream>
#include <format>
#include <vector>

#include <unistd.h>
#include <cstring>

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << std::format("usage: {} <modules_dir> <module_name> <artifacts_dir>", argv[0]) << std::endl;
        return 1;
    }

    try {
        const auto modules_dir = path_t(argv[1]);
        const auto module_name = std::string(argv[2]);
        const auto artifacts_dir = path_t(argv[3]);

        module_graph_t module_graph = module_graph_t::discover(modules_dir, module_name);
        builder_t builder_builder(module_graph, module_graph.builder_module(), artifacts_dir);
        builder_t builder(module_graph, module_graph.target_module(), artifacts_dir);

        const auto cli = filesystem_t::canonical(path_t("/proc/self/exe"));
        const auto cli_last_write_time = filesystem_t::last_write_time(cli);
        const auto cli_version = module_graph_t::derive_version(cli_last_write_time);
        const auto builder_version = module_graph.builder_module().version();

        if (!filesystem_t::exists(builder_builder.libraries_install_dir(library_type_t::SHARED)) || cli_version < builder_version) {
            builder_builder.compile_builder_module_phase(build_phase_t::IMPORT_LIBRARIES);

            const auto new_cli = builder_builder.import_install_dir() / relative_path_t("cli");
            if (!filesystem_t::exists(new_cli)) {
                throw std::runtime_error(std::format("expected updated '{}' to exist but it does not", new_cli.string()));
            }

            std::vector<process_arg_t> process_args;
            process_args.push_back(new_cli);
            for (int i = 1; i < argc; ++i) {
                process_args.push_back(argv[i]);
            }
            process_t::exec(process_args);
        }

        if (!filesystem_t::exists(modules_dir)) {
            throw std::runtime_error(std::format("modules directory does not exist '{}'", modules_dir.string()));
        }

        const bool is_svg_option_enabled = false;
        if (is_svg_option_enabled) {
            const auto svg_dir = artifacts_dir / relative_path_t("sccs");
            module_graph.svg(svg_dir, "0");
        }

        builder.import_libraries();

        if (4 < argc) {
            const auto binary = argv[4];
            const auto binary_dir = builder.import_install_dir();
            const auto binary_location = binary_dir / relative_path_t(binary);
            if (!filesystem_t::exists(binary_location)) {
                throw std::runtime_error(std::format("binary '{}' at location '{}' does not exist", binary, binary_location.string()));
            }

            filesystem_t::current_path(binary_dir);

            std::vector<process_arg_t> process_args;
            process_args.push_back(binary);
            for (int i = 5; i < argc; ++i) {
                process_args.push_back(argv[i]);
            }
            process_t::exec(process_args);
        }
    } catch (const std::exception& e) {
        std::cout << std::format("{}: {}", argv[0], e.what()) << std::endl;
        return 1;
    }

    return 0;
}
