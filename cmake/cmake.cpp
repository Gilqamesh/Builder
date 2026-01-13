#include <builder/cmake/cmake.h>

#include <iostream>
#include <format>

void cmake_t::configure(const path_t& source_dir, const path_t& build_dir, const std::vector<std::pair<std::string, std::string>>& define_key_values) {
    if (!filesystem_t::exists(source_dir)) {
        throw std::runtime_error(std::format("cmake_t::configure: source_dir '{}' does not exist", source_dir.string()));
    }

    if (!filesystem_t::exists(build_dir)) {
        filesystem_t::create_directories(build_dir);
    }

    std::string configure_command = std::format("cmake -S \"{}\" -B \"{}\"", source_dir.string(), build_dir.string());
    for (const auto& define_key_value : define_key_values) {
        configure_command += std::format(" -D{}={}", define_key_value.first, define_key_value.second);
    }

    std::cout << configure_command << std::endl;
    const auto configure_command_result = std::system(configure_command.c_str());
    if (configure_command_result != 0) {
        throw std::runtime_error(std::format("cmake_t::configure: cmake configure command failed with exit code {}", configure_command_result));
    }
}

void cmake_t::build(const path_t& build_dir, std::optional<size_t> n_jobs) {
    if (!filesystem_t::exists(build_dir)) {
        throw std::runtime_error(std::format("cmake_t::build: build_dir '{}' does not exist", build_dir.string()));
    }

    std::string build_command = std::format("cmake --build \"{}\"", build_dir.string());
    if (n_jobs.has_value()) {
        build_command += std::format(" -j{}", n_jobs.value());
    } else {
        build_command += " -j";
    }

    std::cout << build_command << std::endl;
    const auto build_command_result = std::system(build_command.c_str());
    if (build_command_result != 0) {
        throw std::runtime_error(std::format("cmake_t::build: cmake build command failed with exit code {}", build_command_result));
    }
}

void cmake_t::install(const path_t& build_dir) {
    if (!filesystem_t::exists(build_dir)) {
        throw std::runtime_error(std::format("cmake_t::install: build_dir '{}' does not exist", build_dir.string()));
    }

    std::string install_command = std::format("cmake --install \"{}\"", build_dir.string());
    std::cout << install_command << std::endl;

    const auto install_command_result = std::system(install_command.c_str());
    if (install_command_result != 0) {
        throw std::runtime_error(std::format("cmake_t::install: cmake install command failed with exit code {}", install_command_result));
    }
}
