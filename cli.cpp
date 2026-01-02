#include "builder_ctx.h"
#include "internal.h"

#include <iostream>
#include <format>
#include <vector>

#include <unistd.h>
#include <cstring>

#ifndef CLI_VERSION
# define CLI_VERSION 0ull
#endif

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

        builder_ctx_t builder_ctx(builder_dir, modules_dir, module_name, artifacts_dir);

        if (CLI_VERSION < builder_ctx.builder_version()) {
            const auto builder_libraries = builder_ctx.build_builder_libraries(BUNDLE_TYPE_STATIC);
            std::string builder_libraries_str;
            for (const auto& builder_library : builder_libraries) {
                if (builder_libraries_str.empty()) {
                    builder_libraries_str += " ";
                }
                builder_libraries_str += builder_library.string();
            }

            const auto cli_cpp = builder_dir / CLI_CPP;
            const auto command = std::format("clang++ -DCLI_VERSION={} -std=c++23 -o {} {} {} -g", builder_ctx.builder_version(), argv[0], cli_cpp.string(), builder_libraries_str);
            std::cout << command << std::endl;
            const int command_result = std::system(command.c_str());
            if (command_result != 0) {
                throw std::runtime_error(std::format("failed to compile updated cli, command exited with code {}", command_result));
            }

            if (!std::filesystem::exists(argv[0])) {
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
            if (execv(argv[0], exec_args.data()) == -1) {
                throw std::runtime_error(std::format("failed to re-execute updated cli: {}", strerror(errno)));
            }
        }

        if (!std::filesystem::exists(modules_dir)) {
            throw std::runtime_error(std::format("modules directory does not exist '{}'", modules_dir.string()));
        }

        uint32_t module_id = builder_ctx.discover();

        const auto svg_dir = artifacts_dir / "sccs";
        builder_ctx.svg(svg_dir, "0");

        builder_ctx.build_module(module_id);
    } catch (const std::exception& e) {
        std::cout << std::format("{}: {}", argv[0], e.what()) << std::endl;
        return 1;
    }

    return 0;
}
