#include "builder_ctx.h"

#include <iostream>
#include <format>
#include <source_location>
#include <vector>

#include <unistd.h>
#include <cstring>

int main(int argc, char** argv) {
    if (argc != 5) {
        std::cerr << std::format("usage: {} <builder_dir> <modules_dir> <module_name> <artifacts_dir>", argv[0]) << std::endl;
        return 1;
    }

    try {
        const auto builder_dir = std::filesystem::path(argv[1]);
        const auto modules_dir = std::filesystem::path(argv[2]);
        const auto module_name = std::string(argv[3]);
        const auto artifacts_dir = std::filesystem::path(argv[4]);

        const auto root_dir = builder_dir.parent_path().empty() ? "." : builder_dir.parent_path();

        builder_ctx_t builder_ctx(builder_dir, modules_dir, module_name, artifacts_dir);

        const auto cli = std::filesystem::canonical("/proc/self/exe");
        const auto cli_src = builder_dir / std::filesystem::path(std::source_location::current().file_name());

        std::error_code ec;
        const auto cli_last_write_time = std::filesystem::last_write_time(cli, ec);
        if (ec) {
            throw std::runtime_error(std::format("failed to get last write time of cli '{}': {}", cli.string(), ec.message()));
        }

        const auto cli_src_last_write_time = std::filesystem::last_write_time(cli_src, ec);
        if (ec) {
            throw std::runtime_error(std::format("failed to get last write time of cli source '{}': {}", cli_src.string(), ec.message()));
        }

        const auto cli_version = builder_ctx.version(cli_last_write_time);
        const auto cli_src_version = builder_ctx.version(cli_src_last_write_time);
        const auto builder_version = builder_ctx.builder_version();

        if (cli_version < std::min(cli_src_version, builder_version)) {
            const auto builder_libraries = builder_ctx.build_builder_libraries(BUNDLE_TYPE_STATIC);
            std::string builder_libraries_str;
            for (const auto& builder_library : builder_libraries) {
                if (builder_libraries_str.empty()) {
                    builder_libraries_str += " ";
                }
                builder_libraries_str += builder_library.string();
            }

            const auto command = std::format("clang++ -I{} -std=c++23 -o {} {} {} -g", root_dir.string(), cli.string(), cli_src.string(), builder_libraries_str);
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

        uint32_t module_id = builder_ctx.discover();

        const bool is_svg_option_enabled = false;
        if (is_svg_option_enabled) {
            const auto svg_dir = artifacts_dir / "sccs";
            builder_ctx.svg(svg_dir, "0");
        }

        builder_ctx.build_module(module_id);
    } catch (const std::exception& e) {
        std::cout << std::format("{}: {}", argv[0], e.what()) << std::endl;
        return 1;
    }

    return 0;
}
