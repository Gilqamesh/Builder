#include "module.h"
#include "module_builder.h"
#include "module_repository.h"

#include <iostream>
#include <filesystem>
#include <format>
#include <cassert>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << std::format("usage: {} <target_module_directory>", argv[0]) << std::endl;
        exit(1);
    }

    module_t* target_module = nullptr;

    const std::string builder_cpp_filename = "builder.cpp";

    const std::filesystem::path target_module_directory_path = std::filesystem::canonical(argv[1]);
    if (!std::filesystem::exists(target_module_directory_path) || !std::filesystem::is_directory(target_module_directory_path)) {
        std::cerr << std::format("[error] target module directory doesn't exist: {}", target_module_directory_path.string()) << std::endl;
        exit(1);
    }
    const std::filesystem::path target_builder_cpp_path = target_module_directory_path / builder_cpp_filename;
    if (!std::filesystem::exists(target_builder_cpp_path) || !std::filesystem::is_regular_file(target_builder_cpp_path)) {
        std::cerr << std::format("[error] target module's builder file doesn't exist: {}", target_builder_cpp_path.string()) << std::endl;
        exit(1);
    }

    const std::filesystem::path modules_directory_path = target_module_directory_path.parent_path();
    if (target_module_directory_path == modules_directory_path) {
        std::cerr << std::format("[error] no parent modules directory: {}", target_module_directory_path.string()) << std::endl;
        exit(1);
    }

    module_repository_t module_repository;

    std::filesystem::directory_entry directory_entry(modules_directory_path);
    for (const auto& module_directory_path : std::filesystem::directory_iterator(directory_entry)) {
        if (!module_directory_path.is_directory()) {
            continue ;
        }
        
        const std::filesystem::path builder_cpp_path = module_directory_path.path() / builder_cpp_filename;
        if (!std::filesystem::exists(builder_cpp_path)) {
            std::cerr << std::format("[warning] module's builder file doesn't exist: {}", builder_cpp_path.string()) << std::endl;
            continue ;
        }

        const std::string module_name = module_directory_path.path().filename();
        module_t* module = new module_t(module_name, builder_cpp_path);
        module_repository.add_module(module);

        if (module_directory_path.path() == target_module_directory_path) {
            target_module = module;
        }
    }
    assert(target_module != nullptr);

    module_builder_t::populate_dependencies(module_repository, target_module);

    target_module->print_dependencies();

    return 0;
}
