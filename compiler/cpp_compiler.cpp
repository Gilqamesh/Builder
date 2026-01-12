#include <modules/builder/compiler/cpp_compiler.h>

#include <format>
#include <iostream>
#include <ranges>
#include <chrono>
#include <sstream>
#include <fstream>

path_t cpp_compiler_t::create_static_library(
    const builder_t* builder,
    const std::vector<path_t>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const relative_path_t& relative_static_library_path
) {
    return create_static_library(
        builder->build_dir(LIBRARY_TYPE_STATIC),
        builder->src_dir(),
        { builder->include_dir() },
        source_files,
        define_key_values,
        builder->export_dir(LIBRARY_TYPE_STATIC) / relative_static_library_path
    );
}

path_t cpp_compiler_t::create_shared_library(
    const builder_t* builder,
    const std::vector<path_t>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const relative_path_t& relative_shared_library_path
) {
    return create_shared_library(
        builder->build_dir(LIBRARY_TYPE_SHARED),
        builder->src_dir(),
        { builder->include_dir() },
        source_files,
        define_key_values,
        {},
        builder->export_dir(LIBRARY_TYPE_SHARED) / relative_shared_library_path
    );
}

path_t cpp_compiler_t::create_library(
    const builder_t* builder,
    const std::vector<path_t>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const relative_path_t& relative_libary_stem,
    library_type_t library_type
) {
    switch (library_type) {
        case LIBRARY_TYPE_STATIC: return create_static_library(builder, source_files, define_key_values, relative_libary_stem + ".a");
        case LIBRARY_TYPE_SHARED: return create_shared_library(builder, source_files, define_key_values, relative_libary_stem + ".so");
        default: throw std::runtime_error(std::format("cpp_compiler_t::create_library: unknown library_type {}", static_cast<int>(library_type)));
    }
}

path_t cpp_compiler_t::create_binary(
    const builder_t* builder,
    const std::vector<path_t>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    library_type_t library_type,
    const relative_path_t& relative_binary_path
) {
    return create_binary(
        builder->build_dir(library_type),
        builder->src_dir(),
        { builder->include_dir() },
        source_files,
        define_key_values,
        builder->export_libraries(library_type),
        library_type,
        builder->import_dir() / relative_binary_path
    );
}

path_t cpp_compiler_t::reference_static_library(
    const builder_t* builder,
    const path_t& existing_static_library,
    const relative_path_t& relative_static_library_path
) {
    return reference_static_library(existing_static_library, builder->export_dir(LIBRARY_TYPE_STATIC) / relative_static_library_path);
}

path_t cpp_compiler_t::reference_shared_library(
    const builder_t* builder,
    const path_t& existing_shared_library,
    const relative_path_t& relative_shared_library_path
) {
    return reference_shared_library(existing_shared_library, builder->export_dir(LIBRARY_TYPE_SHARED) / relative_shared_library_path);
}

path_t cpp_compiler_t::reference_library(
    const builder_t* builder,
    const path_t& existing_library,
    const relative_path_t& relative_library_path,
    library_type_t library_type
) {
    switch (library_type) {
        case LIBRARY_TYPE_STATIC: return reference_static_library(builder, existing_library, relative_library_path);
        case LIBRARY_TYPE_SHARED: return reference_shared_library(builder, existing_library, relative_library_path);
        default: throw std::runtime_error(std::format("cpp_compiler_t::reference_library: unknown library_type {}", static_cast<int>(library_type)));
    }
}

path_t cpp_compiler_t::reference_binary(
    const builder_t* builder,
    const path_t& existing_binary,
    const relative_path_t& relative_binary_path
) {
    if (!filesystem_t::exists(existing_binary)) {
        throw std::runtime_error(std::format("cpp_compiler_t::reference_binary: referenced binary '{}' does not exist", existing_binary.string()));
    }

    const auto binary_dir = builder->import_dir();
    const auto binary = binary_dir / relative_binary_path;

    filesystem_t::create_symlink(existing_binary, binary);

    return binary;
}

path_t cpp_compiler_t::create_static_library(
    const path_t& cache_dir,
    const path_t& source_dir,
    const std::vector<path_t>& include_dirs,
    const std::vector<path_t>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const path_t& static_library
) {
    const auto object_files = cache_object_files(
        cache_dir,
        source_dir,
        include_dirs,
        source_files,
        define_key_values,
        false
    );

    const auto static_library_dir = static_library.parent();
    if (!filesystem_t::exists(static_library_dir)) {
        filesystem_t::create_directories(static_library_dir);
    }

    std::string command = "ar rcs " + static_library.string();
    for (const auto& object_file : object_files) {
        command += " " + object_file.string();
    }

    std::cout << command << std::endl;
    const int command_result = std::system(command.c_str());
    if (command_result != 0) {
        throw std::runtime_error(std::format("cpp_compiler_t::create_static_library: failed to create static library '{}', command exited with code {}", static_library.string(), command_result));
    }

    if (!filesystem_t::exists(static_library)) {
        throw std::runtime_error(std::format("cpp_compiler_t::create_static_library: expected output static library '{}' to exist but it does not", static_library.string()));
    }

    return static_library;
}

path_t cpp_compiler_t::create_shared_library(
    const path_t& cache_dir,
    const path_t& source_dir,
    const std::vector<path_t>& include_dirs,
    const std::vector<path_t>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const std::vector<path_t>& dsos,
    const path_t& shared_library
) {
    const auto object_files = cache_object_files(
        cache_dir,
        source_dir,
        include_dirs,
        source_files,
        define_key_values,
        true
    );

    const auto shared_library_dir = shared_library.parent();
    if (!filesystem_t::exists(shared_library_dir)) {
        filesystem_t::create_directories(shared_library_dir);
    }

    std::string command = "clang++ -shared -o " + shared_library.string();
    for (const auto& object_file : object_files) {
        command += " " + object_file.string();
    }

    for (const auto& dso : dsos) {
        if (!filesystem_t::exists(dso)) {
            throw std::runtime_error(std::format("cpp_compiler_t::create_shared_library: dso does not exist '{}'", dso.string()));
        }

        command += " " + dso.string();
    }

    std::cout << command << std::endl;
    const int command_result = std::system(command.c_str());
    if (command_result != 0) {
        throw std::runtime_error(std::format("cpp_compiler_t::create_shared_library: failed to create shared library '{}', command exited with code {}", shared_library.string(), command_result));
    }

    if (!filesystem_t::exists(shared_library)) {
        throw std::runtime_error(std::format("cpp_compiler_t::create_shared_library: expected output shared library '{}' to exist but it does not", shared_library.string()));
    }

    return shared_library;
}

path_t cpp_compiler_t::create_binary(
    const path_t& cache_dir,
    const path_t& source_dir,
    const std::vector<path_t>& include_dirs,
    const std::vector<path_t>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const std::vector<std::vector<path_t>>& library_groups,
    library_type_t library_type,
    const path_t& binary
) {
    const auto object_files = cache_object_files(cache_dir, source_dir, include_dirs, source_files, define_key_values, library_type);

    const auto binary_dir = binary.parent();
    if (!filesystem_t::exists(binary_dir)) {
        filesystem_t::create_directories(binary_dir);
    }

    std::string command = std::format("clang++ -g -std=c++23 -o {}", binary.string());
    for (const auto& object_file : object_files) {
        command += " " + object_file.string();
    }

    // TODO: mixed static/shared interleave of start/end group, typify 'library_groups'
    for (auto library_group_it = library_groups.rbegin(); library_group_it != library_groups.rend(); ++library_group_it) {
        if (library_type != LIBRARY_TYPE_SHARED && 1 < library_group_it->size()) {
            command += " -Wl,--start-group";
        }
        for (const auto& library : *library_group_it) {
            if (!filesystem_t::exists(library)) {
                throw std::runtime_error(std::format("cpp_compiler_t::create_binary: library does not exist '{}'", library.string()));
            }
            command += " " + library.string();
        }
        if (library_type != LIBRARY_TYPE_SHARED && 1 < library_group_it->size()) {
            command += " -Wl,--end-group";
        }
    }

    std::cout << command << std::endl;
    const int command_result = std::system(command.c_str());
    if (command_result != 0) {
        throw std::runtime_error(std::format("cpp_compiler_t::create_binary: failed to create binary '{}', command exited with code {}", binary.string(), command_result));
    }

    if (!filesystem_t::exists(binary)) {
        throw std::runtime_error(std::format("cpp_compiler_t::create_binary: expected output binary '{}' to exist but it does not", binary.string()));
    }

    return binary;
}

path_t cpp_compiler_t::reference_static_library(
    const path_t& existing_static_library,
    const path_t& static_library
) {
    if (!filesystem_t::exists(existing_static_library)) {
        throw std::runtime_error(std::format("cpp_compiler_t::reference_static_library: referenced static library '{}' does not exist", existing_static_library.string()));
    }

    const auto static_library_dir = static_library.parent();
    if (!filesystem_t::exists(static_library_dir)) {
        filesystem_t::create_directories(static_library_dir);
    }

    filesystem_t::create_symlink(existing_static_library, static_library);

    return static_library;
}

path_t cpp_compiler_t::reference_shared_library(
    const path_t& existing_shared_library,
    const path_t& shared_library
) {
    if (!filesystem_t::exists(existing_shared_library)) {
        throw std::runtime_error(std::format("cpp_compiler_t::reference_shared_library: referenced shared library '{}' does not exist", existing_shared_library.string()));
    }

    const auto shared_library_dir = shared_library.parent();
    if (!filesystem_t::exists(shared_library_dir)) {
        filesystem_t::create_directories(shared_library_dir);
    }

    filesystem_t::create_symlink(existing_shared_library, shared_library);

    return shared_library;
}

path_t cpp_compiler_t::reference_shared_library(
    const path_t& existing_library,
    const path_t& library_stem,
    library_type_t library_type
) {
    switch (library_type) {
        case LIBRARY_TYPE_STATIC: return reference_static_library(existing_library, library_stem + ".a");
        case LIBRARY_TYPE_SHARED: return reference_shared_library(existing_library, library_stem + ".so");
        default: throw std::runtime_error(std::format("cpp_compiler_t::reference_shared_library: unknown library_type {}", static_cast<int>(library_type)));
    }
}

std::vector<path_t> cpp_compiler_t::cache_object_files(
    const builder_t* builder,
    const std::vector<path_t>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    library_type_t library_type
) {
    return cache_object_files(
        builder->build_dir(library_type),
        builder->src_dir(),
        { builder->include_dir() },
        source_files,
        define_key_values,
        library_type == LIBRARY_TYPE_SHARED
    );
}

std::vector<path_t> cpp_compiler_t::cache_object_files(
    const path_t& cache_dir,
    const path_t& source_dir,
    const std::vector<path_t>& include_dirs,
    const std::vector<path_t>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    bool position_independent
) {
    std::vector<path_t> result;
    result.reserve(source_files.size());

    if (!filesystem_t::exists(cache_dir)) {
        filesystem_t::create_directories(cache_dir);
    }

    std::string define_flags_str;
    for (const auto& [key, value] : define_key_values) {
        define_flags_str += std::format("-D{}={} ", key, value);
    }

    std::string include_dirs_str;
    for (const auto& include_dir : include_dirs) {
        include_dirs_str += std::format("-I{} ", include_dir.string());
    }

    for (const auto& source_file : source_files) {
        if (!filesystem_t::exists(source_file)) {
            throw std::runtime_error(std::format("cpp_compiler_t::cache_object_files: source file does not exist '{}'", source_file.string()));
        }

        const auto rel = source_dir.relative(source_file);
        auto object_file = cache_dir / rel;
        object_file.extension(".o");

        const auto object_file_dir = object_file.parent();
        if (!filesystem_t::exists(object_file_dir)) {
            filesystem_t::create_directories(object_file_dir);
        }

        std::string compile_command = std::format("clang++ {}{}-g {}-std=c++23 -c {} -o {}", include_dirs_str, define_flags_str, position_independent ? "-fPIC " : "", source_file.string(), object_file.string());
        std::cout << compile_command << std::endl;
        const int compile_command_result = std::system(compile_command.c_str());
        if (compile_command_result != 0) {
            throw std::runtime_error(std::format("cpp_compiler_t::cache_object_files: failed to compile '{}', compilation result: {}", source_file.string(), compile_command_result));
        }

        result.push_back(object_file);
    }

    return result;
}
