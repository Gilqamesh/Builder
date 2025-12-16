#include "compiler.h"

#include <format>
#include <iostream>

std::filesystem::path compiler_t::update_object_file(
    const std::filesystem::path& source_file,
    const std::vector<std::filesystem::path>& header_files,
    const std::vector<std::filesystem::path>& include_dirs,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const std::filesystem::path& output_o_file,
    bool position_independent
) {
    if (!std::filesystem::exists(source_file)) {
        throw std::runtime_error(std::format("source file does not exist '{}'", source_file.string()));
    }

    std::filesystem::file_time_type latest_input_file_time = std::filesystem::last_write_time(source_file);
    for (const auto& header_file : header_files) {
        if (!std::filesystem::exists(header_file)) {
            throw std::runtime_error(std::format("header file does not exist '{}'", header_file.string()));
        }
        latest_input_file_time = std::max(latest_input_file_time, std::filesystem::last_write_time(header_file));
    }

    if (std::filesystem::exists(output_o_file) && latest_input_file_time <= std::filesystem::last_write_time(output_o_file)) {
        return output_o_file;
    }

    std::string define_flags;
    for (const auto& [key, value] : define_key_values) {
        define_flags += std::format("-D{}={} ", key, value);
    }

    std::string compile_command = std::format("clang++ {}-g {}-std=c++23 -c {} -o {}", define_flags.empty() ? "" : define_flags, position_independent ? "-fPIC " : "", source_file.string(), output_o_file.string());
    for (const auto& include_dir : include_dirs) {
        compile_command += " -I" + include_dir.string();
    }

    std::cout << compile_command << std::endl;
    const int result = std::system(compile_command.c_str());
    if (result != 0) {
        throw std::runtime_error(std::format("failed to compile '{}'", source_file.string()));
    }

    return output_o_file;
}

std::filesystem::path compiler_t::update_static_library(
    const std::vector<std::filesystem::path>& input_files,
    const std::filesystem::path& output_static_library
) {
    std::filesystem::file_time_type latest_input_file_time = std::filesystem::file_time_type::min();
    std::string archive_command = "ar rcs " + output_static_library.string();
    for (const auto& input_file : input_files) {
        if (!std::filesystem::exists(input_file)) {
            throw std::runtime_error(std::format("input file does not exist '{}'", input_file.string()));
        }
        latest_input_file_time = std::max(latest_input_file_time, std::filesystem::last_write_time(input_file));
        archive_command += " " + input_file.string();
    }

    if (std::filesystem::exists(output_static_library) && latest_input_file_time <= std::filesystem::last_write_time(output_static_library)) {
        return output_static_library;
    }

    std::cout << archive_command << std::endl;
    const int result = std::system(archive_command.c_str());
    if (result != 0) {
        throw std::runtime_error(std::format("failed to create archive '{}'", output_static_library.string()));
    }

    return output_static_library;
}

std::filesystem::path compiler_t::update_shared_libary(
    const std::vector<std::filesystem::path>& input_files,
    const std::filesystem::path& output_shared_libary
) {
    std::filesystem::file_time_type latest_input_file_time = std::filesystem::file_time_type::min();
    std::string link_command = "clang++ -shared -o " + output_shared_libary.string();
    for (const auto& input_file : input_files) {
        if (!std::filesystem::exists(input_file)) {
            throw std::runtime_error(std::format("input file does not exist '{}'", input_file.string()));
        }
        latest_input_file_time = std::max(latest_input_file_time, std::filesystem::last_write_time(input_file));
        link_command += " " + input_file.string();
    }

    if (std::filesystem::exists(output_shared_libary) && latest_input_file_time <= std::filesystem::last_write_time(output_shared_libary)) {
        return output_shared_libary;
    }

    std::cout << link_command << std::endl;
    const int result = std::system(link_command.c_str());
    if (result != 0) {
        throw std::runtime_error(std::format("failed to create shared library '{}'", output_shared_libary.string()));
    }

    return output_shared_libary;
}

bool compiler_t::update_binary(
    const std::vector<std::filesystem::path>& input_libraries,
    const std::filesystem::path& output_binary
) {
    std::filesystem::file_time_type latest_input_library_time = std::filesystem::file_time_type::min();
    std::string link_command = "clang++ -o " + output_binary.string();
    for (const auto& input_library : input_libraries) {
        if (!std::filesystem::exists(input_library)) {
            throw std::runtime_error(std::format("input file does not exist '{}'", input_library.string()));
        }
        latest_input_library_time = std::max(latest_input_library_time, std::filesystem::last_write_time(input_library));
        link_command += " " + input_library.string();
    }

    if (std::filesystem::exists(output_binary) && latest_input_library_time <= std::filesystem::last_write_time(output_binary)) {
        return false;
    }

    std::cout << link_command << std::endl;
    const int result = std::system(link_command.c_str());
    if (result != 0) {
        throw std::runtime_error(std::format("failed to create binary '{}'", output_binary.string()));
    }

    return true;
}
