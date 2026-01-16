#include "module_builder.h"
#include "process.h"

#include <iostream>
#include <format>
#include <vector>

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << std::format("usage: {} <modules_dir> <module_name> <artifacts_dir>", argv[0]) << std::endl;
        return 1;
    }

    try {
        const auto modules_dir = filesystem::path_t(argv[1]);
        const auto module_name = std::string(argv[2]);
        const auto artifacts_dir = filesystem::path_t(argv[3]);

        builder::module_graph_t module_graph = builder::module_graph_t::discover(modules_dir, module_name);
        builder::module_builder_t builder_module_builder(module_graph, module_graph.builder_module(), artifacts_dir);
        builder::module_builder_t target_module_builder(module_graph, module_graph.target_module(), artifacts_dir);

        const auto cli = filesystem::canonical(filesystem::path_t("/proc/self/exe"));
        const auto cli_last_write_time = filesystem::last_write_time(cli);
        const auto cli_version = builder::module_graph_t::derive_version(cli_last_write_time);
        const auto builder_version = module_graph.builder_module().version();

        if (!filesystem::exists(builder_module_builder.libraries_install_dir(builder::library_type_t::SHARED)) || cli_version < builder_version) {
            builder_module_builder.compile_builder_module_phase(builder::module_builder_t::phase_t::IMPORT_LIBRARIES);

            const auto new_cli = builder_module_builder.import_install_dir() / filesystem::relative_path_t("cli");
            if (!filesystem::exists(new_cli)) {
                throw std::runtime_error(std::format("expected updated '{}' to exist but it does not", new_cli));
            }

            std::vector<process::process_arg_t> process_args;
            process_args.push_back(new_cli);
            for (int i = 1; i < argc; ++i) {
                process_args.push_back(argv[i]);
            }
            process::exec(process_args);
        }

        if (!filesystem::exists(modules_dir)) {
            throw std::runtime_error(std::format("modules directory does not exist '{}'", modules_dir));
        }

        const bool is_svg_option_enabled = false;
        if (is_svg_option_enabled) {
            const auto svg_dir = artifacts_dir / filesystem::relative_path_t("sccs");
            module_graph.svg(svg_dir, "0");
        }

        target_module_builder.import_libraries();

        if (4 < argc) {
            const auto binary = argv[4];
            const auto binary_dir = target_module_builder.import_install_dir();
            const auto binary_location = binary_dir / filesystem::relative_path_t(binary);
            if (!filesystem::exists(binary_location)) {
                throw std::runtime_error(std::format("binary '{}' at location '{}' does not exist", binary, binary_location));
            }

            filesystem::current_path(binary_dir);

            std::vector<process::process_arg_t> process_args;
            process_args.push_back(binary);
            for (int i = 5; i < argc; ++i) {
                process_args.push_back(argv[i]);
            }
            process::exec(process_args);
        }
    } catch (const std::exception& e) {
        std::cout << std::format("{}: {}", argv[0], e.what()) << std::endl;
        return 1;
    }

    return 0;
}
