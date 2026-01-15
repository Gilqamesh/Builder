#include "cpp_compiler.h"
#include "../process/process.h"

#include <format>
#include <ranges>
#include <chrono>
#include <sstream>
#include <fstream>

namespace cpp_compiler {

static std::vector<filesystem::path_t> create_object_files(
    const filesystem::path_t& cache_dir,
    const filesystem::path_t& source_dir,
    const std::vector<filesystem::path_t>& include_dirs,
    const std::vector<filesystem::path_t>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    bool is_position_independent
) {
    std::vector<filesystem::path_t> result;
    result.reserve(source_files.size());

    if (!filesystem::exists(cache_dir)) {
        filesystem::create_directories(cache_dir);
    }

    std::vector<process::process_arg_t> process_prefix_args;
    process_prefix_args.push_back(CPP_COMPILER_PATH);
    process_prefix_args.push_back("-g");
    process_prefix_args.push_back("-std=c++23");

    std::string define_flags_str;
    for (const auto& [key, value] : define_key_values) {
        process_prefix_args.push_back(std::format("-D{}={}", key, value));
    }

    std::string include_dirs_str;
    for (const auto& include_dir : include_dirs) {
        process_prefix_args.push_back(std::format("-I{}", include_dir));
    }

    for (const auto& source_file : source_files) {
        if (!filesystem::exists(source_file)) {
            throw std::runtime_error(std::format("cpp_compiler::create_object_files: source file does not exist '{}'", source_file));
        }

        const auto rel = source_dir.relative(source_file);
        auto object_file = cache_dir / rel;
        object_file.extension(".o");

        const auto object_file_dir = object_file.parent();
        if (!filesystem::exists(object_file_dir)) {
            filesystem::create_directories(object_file_dir);
        }

        std::vector<process::process_arg_t> process_args = process_prefix_args;
        if (is_position_independent) {
            process_args.push_back("-fPIC");
        }
        process_args.push_back("-c");
        process_args.push_back(source_file);
        process_args.push_back("-o");
        process_args.push_back(object_file);

        const int process_result = process::create_and_wait(process_args);
        if (process_result != 0) {
            throw std::runtime_error(std::format("cpp_compiler::create_object_files: failed to compile '{}', compilation result: {}", source_file, process_result));
        }

        result.push_back(object_file);
    }

    return result;
}

filesystem::path_t create_static_library(
    const filesystem::path_t& cache_dir,
    const filesystem::path_t& source_dir,
    const std::vector<filesystem::path_t>& include_dirs,
    const std::vector<filesystem::path_t>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const filesystem::path_t& static_library
) {
    const auto object_files = create_object_files(
        cache_dir,
        source_dir,
        include_dirs,
        source_files,
        define_key_values,
        false
    );

    const auto static_library_dir = static_library.parent();
    if (!filesystem::exists(static_library_dir)) {
        filesystem::create_directories(static_library_dir);
    }

    std::vector<process::process_arg_t> process_args;
    process_args.push_back(AR_PATH);
    process_args.push_back("rcs");
    process_args.push_back(static_library);
    for (const auto& object_file : object_files) {
        process_args.push_back(object_file);
    }

    const int process_result = process::create_and_wait(process_args);
    if (process_result != 0) {
        throw std::runtime_error(std::format("cpp_compiler::create_static_library: failed to create static library '{}', command exited with code {}", static_library, process_result));
    }

    if (!filesystem::exists(static_library)) {
        throw std::runtime_error(std::format("cpp_compiler::create_static_library: expected output static library '{}' to exist but it does not", static_library));
    }

    return static_library;
}

filesystem::path_t create_shared_library(
    const filesystem::path_t& cache_dir,
    const filesystem::path_t& source_dir,
    const std::vector<filesystem::path_t>& include_dirs,
    const std::vector<filesystem::path_t>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const std::vector<filesystem::path_t>& dsos,
    const filesystem::path_t& shared_library
) {
    const auto object_files = create_object_files(
        cache_dir,
        source_dir,
        include_dirs,
        source_files,
        define_key_values,
        true
    );

    const auto shared_library_dir = shared_library.parent();
    if (!filesystem::exists(shared_library_dir)) {
        filesystem::create_directories(shared_library_dir);
    }

    std::vector<process::process_arg_t> process_args;
    process_args.push_back(CPP_COMPILER_PATH);
    process_args.push_back("-g");
    process_args.push_back("-shared");
    process_args.push_back("-o");
    process_args.push_back(shared_library);
    for (const auto& object_file : object_files) {
        process_args.push_back(object_file);
    }

    for (const auto& dso : dsos) {
        if (!filesystem::exists(dso)) {
            throw std::runtime_error(std::format("cpp_compiler::create_shared_library: dso does not exist '{}'", dso));
        }

        process_args.push_back(dso);
    }

    const int process_result = process::create_and_wait(process_args);
    if (process_result != 0) {
        throw std::runtime_error(std::format("cpp_compiler::create_shared_library: failed to create shared library '{}', command exited with code {}", shared_library, process_result));
    }

    if (!filesystem::exists(shared_library)) {
        throw std::runtime_error(std::format("cpp_compiler::create_shared_library: expected output shared library '{}' to exist but it does not", shared_library));
    }

    return shared_library;
}

filesystem::path_t create_binary(
    const filesystem::path_t& cache_dir,
    const filesystem::path_t& source_dir,
    const std::vector<filesystem::path_t>& include_dirs,
    const std::vector<filesystem::path_t>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const std::vector<std::vector<filesystem::path_t>>& library_groups,
    bool TEMP_assume_all_link_inputs_are_shared,
    const filesystem::path_t& binary
) {
    const auto object_files = create_object_files(cache_dir, source_dir, include_dirs, source_files, define_key_values, TEMP_assume_all_link_inputs_are_shared);

    const auto binary_dir = binary.parent();
    if (!filesystem::exists(binary_dir)) {
        filesystem::create_directories(binary_dir);
    }

    std::vector<process::process_arg_t> process_args;
    process_args.push_back(CPP_COMPILER_PATH);
    process_args.push_back("-g");
    process_args.push_back("-std=c++23");
    process_args.push_back("-o");
    process_args.push_back(binary);
    for (const auto& object_file : object_files) {
        process_args.push_back(object_file);
    }

    // TODO: mixed static/shared interleave of start/end group, typify 'library_groups'
    for (auto library_group_it = library_groups.rbegin(); library_group_it != library_groups.rend(); ++library_group_it) {
        if (!TEMP_assume_all_link_inputs_are_shared && 1 < library_group_it->size()) {
            process_args.push_back("-Wl,--start-group");
        }
        for (const auto& library : *library_group_it) {
            if (!filesystem::exists(library)) {
                throw std::runtime_error(std::format("cpp_compiler::create_binary: library does not exist '{}'", library));
            }
            process_args.push_back(library);
        }
        if (!TEMP_assume_all_link_inputs_are_shared && 1 < library_group_it->size()) {
            process_args.push_back("-Wl,--end-group");
        }
    }

    for (const auto& library_group : library_groups) {
        for (const auto& library_group : library_group) {
            process_args.push_back(std::format("-Wl,-rpath,{}", library_group.parent()));
        }
    }
    
    const int process_result = process::create_and_wait(process_args);
    if (process_result != 0) {
        throw std::runtime_error(std::format("cpp_compiler::create_binary: failed to create binary '{}', command exited with code {}", binary, process_result));
    }

    if (!filesystem::exists(binary)) {
        throw std::runtime_error(std::format("cpp_compiler::create_binary: expected output binary '{}' to exist but it does not", binary));
    }

    return binary;
}

} // namespace cpp_compiler
