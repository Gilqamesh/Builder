#include "module.h"

#include <iostream>
#include <filesystem>
#include <format>
#include <string>
#include <unordered_map>
#include <stack>
#include <regex>
#include <fstream>

struct command_line_parser_t {
    command_line_parser_t(int argc, char** argv):
        artifacts_root("artifacts"),
        modules_root("modules"),
        module_name("")
    {
        for (int iarg = 1; iarg < argc; ++iarg) {
            const std::string arg = std::string(argv[iarg]);
            if (arg == "-B" && iarg + 1 < argc) {
                artifacts_root = std::filesystem::canonical(argv[++iarg]);
            } else if (arg == "-S" && iarg + 1 < argc) {
                modules_root = std::filesystem::canonical(argv[++iarg]);
            } else if (arg == "--help" || arg == "-h") {
                print_usage(argv[0]);
                exit(0);
            } else {
                module_name = arg;
            }
        }

        if (!std::filesystem::exists(artifacts_root)) {
            if (!std::filesystem::create_directories(artifacts_root)) {
                std::cerr << std::format("[error] failed to create artifacts root directory: {}", artifacts_root.string()) << std::endl;
                exit(1);
            }
        } else if (!std::filesystem::is_directory(artifacts_root)) {
            std::cerr << std::format("[error] artifacts root path is not a directory: {}", artifacts_root.string()) << std::endl;
            exit(1);
        }
        artifacts_root = std::filesystem::canonical(artifacts_root);

        if (!std::filesystem::exists(modules_root)) {
            std::cerr << std::format("[error] modules root directory doesn't exist: {}", modules_root.string()) << std::endl;
            exit(1);
        } else if (!std::filesystem::is_directory(modules_root)) {
            std::cerr << std::format("[error] modules root path is not a directory: {}", modules_root.string()) << std::endl;
            exit(1);
        }
        modules_root = std::filesystem::canonical(modules_root);

        if (module_name.empty()) {
            print_usage(argv[0]);
            exit(1);
        }
    }

    void print_usage(const char* program_name) const {
        std::cout << std::format("usage: {} [-B <artifacts_root>] [-M <modules_root>] <module_name>", program_name) << std::endl;
    }

    std::filesystem::path artifacts_root;
    std::filesystem::path modules_root;
    std::string module_name;
};

int main(int argc, char** argv) {
    const command_line_parser_t cmd_parser(argc, argv);

    module_t* target_module = nullptr;

    std::unordered_map<std::string, module_t*> module_by_name;

    const std::string builder_cpp_filename = "builder.cpp";

    const std::filesystem::path target_module_directory = cmd_parser.modules_root / cmd_parser.module_name;
    if (!std::filesystem::exists(target_module_directory)) {
        std::cerr << std::format("[error] target module directory doesn't exist: {}", target_module_directory.string()) << std::endl;
        exit(1);
    } else if (!std::filesystem::is_directory(target_module_directory)) {
        std::cerr << std::format("[error] target module directory is not a directory: {}", target_module_directory.string()) << std::endl;
        exit(1);
    }

    const std::filesystem::path target_builder_cpp = target_module_directory / builder_cpp_filename;
    if (!std::filesystem::exists(target_builder_cpp)) {
        std::cerr << std::format("[error] target module builder file doesn't exist: {}", target_builder_cpp.string()) << std::endl;
        exit(1);
    } else if (!std::filesystem::is_regular_file(target_builder_cpp)) {
        std::cerr << std::format("[error] target module builder file is not a regular file: {}", target_builder_cpp.string()) << std::endl;
        exit(1);
    }

    for (const auto& module_directory : std::filesystem::directory_iterator(std::filesystem::directory_entry(cmd_parser.modules_root))) {
        if (!module_directory.is_directory()) {
            continue ;
        }

        const std::filesystem::path builder_cpp = module_directory.path() / builder_cpp_filename;
        if (!std::filesystem::exists(builder_cpp)) {
            continue ;
        } else if (!std::filesystem::is_regular_file(builder_cpp)) {
            std::cerr << std::format("[warning] module builder file is not a regular file: {}", builder_cpp.string()) << std::endl;
            continue ;
        }

        const std::string module_name = module_directory.path().filename();
        module_t* module = new module_t(module_name, builder_cpp, cmd_parser.artifacts_root, cmd_parser.modules_root);
        module_by_name.emplace(module_name, module);

        if (module_directory.path() == target_module_directory) {
            target_module = module;
        }
    }

    if (target_module == nullptr) {
        std::cerr << std::format("[error] module not found in repository: {}", target_module_directory.string()) << std::endl;
        exit(1);
    }

    std::ostream& os = std::cout;

    target_module->construct(os, module_by_name);
    target_module->build(os);
    target_module->run(os);

    return 0;
}
