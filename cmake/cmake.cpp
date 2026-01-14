#include "cmake.h"
#include "../process/process.h"

#include <iostream>
#include <format>

void cmake_t::configure(const path_t& source_dir, const path_t& build_dir, const std::vector<std::pair<std::string, std::string>>& define_key_values) {
    if (!filesystem_t::exists(source_dir)) {
        throw std::runtime_error(std::format("cmake_t::configure: source_dir '{}' does not exist", source_dir));
    }

    if (!filesystem_t::exists(build_dir)) {
        filesystem_t::create_directories(build_dir);
    }

    std::vector<process_arg_t> configure_process_args;
    configure_process_args.push_back(CMAKE_PATH);
    configure_process_args.push_back("-S");
    configure_process_args.push_back(source_dir);
    configure_process_args.push_back("-B");
    configure_process_args.push_back(build_dir);

    for (const auto& define_key_value : define_key_values) {
        configure_process_args.push_back(std::format("-D{}={}", define_key_value.first, define_key_value.second));
    }

    const int configure_process_result = process_t::create_and_wait(configure_process_args);
    if (0 < configure_process_result) {
        throw std::runtime_error(std::format("cmake_t::configure: cmake configure command failed with exit code {}", configure_process_result));
    } else if (configure_process_result < 0) {
        throw std::runtime_error(std::format("cmake_t::configure: cmake configure command terminated by signal {}", -configure_process_result));
    }
}

void cmake_t::build(const path_t& build_dir, std::optional<size_t> n_jobs) {
    if (!filesystem_t::exists(build_dir)) {
        throw std::runtime_error(std::format("cmake_t::build: build_dir '{}' does not exist", build_dir));
    }

    std::vector<process_arg_t> build_process_args;
    build_process_args.push_back(CMAKE_PATH);
    build_process_args.push_back("--build");
    build_process_args.push_back(build_dir);

    if (n_jobs.has_value()) {
        build_process_args.push_back(std::format("-j{}", n_jobs.value()));
    } else {
        build_process_args.push_back("-j");
    }

    const int build_process_result = process_t::create_and_wait(build_process_args);
    if (0 < build_process_result) {
        throw std::runtime_error(std::format("cmake_t::build: cmake build command failed with exit code {}", build_process_result));
    } else if (build_process_result < 0) {
        throw std::runtime_error(std::format("cmake_t::build: cmake build command terminated by signal {}", -build_process_result));
    }
}

void cmake_t::install(const path_t& build_dir) {
    if (!filesystem_t::exists(build_dir)) {
        throw std::runtime_error(std::format("cmake_t::install: build_dir '{}' does not exist", build_dir));
    }

    const int install_process_result = process_t::create_and_wait({
        CMAKE_PATH,
        "--install",
        build_dir
    });
    if (0 < install_process_result) {
        throw std::runtime_error(std::format("cmake_t::install: cmake install command failed with exit code {}", install_process_result));
    } else if (install_process_result < 0) {
        throw std::runtime_error(std::format("cmake_t::install: cmake install command terminated by signal {}", -install_process_result));
    }
}
