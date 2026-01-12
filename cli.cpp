#include "module/module_graph.h"

#include <iostream>
#include <format>
#include <source_location>
#include <vector>

#include <unistd.h>
#include <cstring>

int main(int argc, char** argv) {
    if (argc < 4) {
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
        const auto cli_src = builder_builder.src_dir() / relative_path_t(std::filesystem::path(std::source_location::current().file_name()).filename());

        std::error_code ec;
        const auto cli_last_write_time = std::filesystem::last_write_time(cli, ec);
        if (ec) {
            throw std::runtime_error(std::format("failed to get last write time of cli '{}': {}", cli.string(), ec.message()));
        }

        const auto cli_src_last_write_time = filesystem_t::last_write_time(cli_src);

        const auto cli_version = module_graph_t::derive_version(cli_last_write_time);
        const auto cli_src_version = module_graph_t::derive_version(cli_src_last_write_time);
        const auto builder_version = module_graph.builder_module().version();

        if (!filesystem_t::exists(builder_builder.export_dir(LIBRARY_TYPE_SHARED)) || cli_version < std::max(cli_src_version, builder_version)) {
            const auto builder_libraries = builder_builder.export_libraries(LIBRARY_TYPE_SHARED);
            std::string builder_libraries_str;
            for (const auto& builder_library_group : builder_libraries) {
                for (const auto& builder_library : builder_library_group) {
                    if (!builder_libraries_str.empty()) {
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

        if (4 < argc) {
            const auto binary = argv[4];
            const auto binary_dir = builder.import_dir();
            const auto binary_location = binary_dir / relative_path_t(binary);
            if (!filesystem_t::exists(binary_location)) {
                throw std::runtime_error(std::format("binary '{}' at location '{}' does not exist", binary, binary_location.string()));
            }

            std::cout << std::format("cd {}", binary_dir.string()) << std::endl;
            if (chdir(binary_dir.c_str()) != 0) {
                throw std::runtime_error(std::format("failed to change directory to '{}': {}", binary_dir.string(), strerror(errno)));
            }

            std::vector<std::string> exec_string_args;
            exec_string_args.push_back(binary);
            for (int i = 5; i < argc; ++i) {
                exec_string_args.push_back(argv[i]);
            }
            std::string exec_command;
            std::vector<char*> exec_args;
            for (const auto& arg : exec_string_args) {
                exec_args.push_back(const_cast<char*>(arg.c_str()));
                if (!exec_command.empty()) {
                    exec_command += " ";
                }
                exec_command += arg;
            }
            exec_args.push_back(nullptr);

            std::cout << exec_command << std::endl;
            if (execv(binary, exec_args.data()) == -1) {
                throw std::runtime_error(std::format("failed to execute binary '{}': {}", binary, strerror(errno)));
            }
        }
    } catch (const std::exception& e) {
        std::cout << std::format("{}: {}", argv[0], e.what()) << std::endl;
        return 1;
    }

    return 0;
}
