#include "module/module_graph.h"

#include <iostream>
#include <format>
#include <source_location>
#include <vector>

#include <unistd.h>
#include <cstring>

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << std::format("usage: {} <modules_dir> <module_name> <artifacts_dir>", argv[0]) << std::endl;
        return 1;
    }

    try {
        const auto modules_dir = std::filesystem::path(argv[1]);
        const auto module_name = std::string(argv[2]);
        const auto artifacts_dir = std::filesystem::path(argv[3]);

        const auto include_dir = modules_dir.parent_path().empty() ? "." : modules_dir.parent_path();

        module_graph_t module_graph = module_graph_t::discover(modules_dir, module_name);
        builder_t builder_builder(module_graph, module_graph.builder_module(), artifacts_dir);
        builder_t builder(module_graph, module_graph.target_module(), artifacts_dir);

        const auto cli = std::filesystem::canonical("/proc/self/exe");
        const auto cli_src = builder_builder.src_dir() / std::filesystem::path(std::source_location::current().file_name()).filename();

        std::error_code ec;
        const auto cli_last_write_time = std::filesystem::last_write_time(cli, ec);
        if (ec) {
            throw std::runtime_error(std::format("failed to get last write time of cli '{}': {}", cli.string(), ec.message()));
        }

        const auto cli_src_last_write_time = std::filesystem::last_write_time(cli_src, ec);
        if (ec) {
            throw std::runtime_error(std::format("failed to get last write time of cli source '{}': {}", cli_src.string(), ec.message()));
        }

        const auto cli_version = module_graph_t::version(cli_last_write_time);
        const auto cli_src_version = module_graph_t::version(cli_src_last_write_time);
        const auto builder_version = module_graph.builder_module().version();

        if (cli_version < std::max(cli_src_version, builder_version)) {
            const auto builder_libraries = builder_builder.export_libraries(LIBRARY_TYPE_SHARED);
            std::string builder_libraries_str;
            for (const auto& builder_library_group : builder_libraries) {
                for (const auto& builder_library : builder_library_group) {
                    if (builder_libraries_str.empty()) {
                        builder_libraries_str += " ";
                    }
                    builder_libraries_str += builder_library.string();
                }
            }

            const auto command = std::format("clang++ -I{} -std=c++23 -o {} {} {} -g", include_dir.string(), cli.string(), cli_src.string(), builder_libraries_str);
            std::cout << command << std::endl;
            const int command_result = std::system(command.c_str());
            if (command_result != 0) {
                throw std::runtime_error(std::format("failed to compile updated cli, command exited with code {}", command_result));
            }

            if (!std::filesystem::exists(cli.string())) {
                throw std::runtime_error("expected updated cli to exist but it does not");
            }

            std::string exec_command;
            std::vector<char*> exec_args;
            for (int i = 0; i < argc; ++i) {
                exec_args.push_back(argv[i]);
                if (!exec_command.empty()) {
                    exec_command += " ";
                }
                exec_command += argv[i];
            }
            exec_args.push_back(nullptr);

            std::cout << exec_command << std::endl;
            if (execv(cli.c_str(), exec_args.data()) == -1) {
                throw std::runtime_error(std::format("failed to re-execute updated cli: {}", strerror(errno)));
            }
        }

        if (!std::filesystem::exists(modules_dir)) {
            throw std::runtime_error(std::format("modules directory does not exist '{}'", modules_dir.string()));
        }

        const bool is_svg_option_enabled = false;
        if (is_svg_option_enabled) {
            const auto svg_dir = artifacts_dir / "sccs";
            module_graph.svg(svg_dir, "0");
        }

        builder.import_libraries();
    } catch (const std::exception& e) {
        std::cout << std::format("{}: {}", argv[0], e.what()) << std::endl;
        return 1;
    }

    return 0;
}
