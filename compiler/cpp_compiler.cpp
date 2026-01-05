#include <modules/builder/compiler/cpp_compiler.h>

#include <format>
#include <iostream>
#include <ranges>
#include <chrono>
#include <sstream>
#include <fstream>

std::filesystem::path cpp_compiler_t::create_static_library(
    const builder_t* builder,
    const std::vector<std::filesystem::path>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const std::string& static_library_name
) {
    return create_static_library(
        builder->build_dir(LIBRARY_TYPE_STATIC),
        builder->src_dir(),
        { builder->include_dir() },
        source_files,
        define_key_values,
        builder->export_dir(LIBRARY_TYPE_STATIC) / static_library_name
    );
}

std::filesystem::path cpp_compiler_t::create_shared_library(
    const builder_t* builder,
    const std::vector<std::filesystem::path>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const std::string& shared_library_name
) {
    return create_shared_library(
        builder->build_dir(LIBRARY_TYPE_SHARED),
        builder->src_dir(),
        { builder->include_dir() },
        source_files,
        define_key_values,
        {},
        builder->export_dir(LIBRARY_TYPE_SHARED) / shared_library_name
    );
}

std::filesystem::path cpp_compiler_t::create_library(
    const builder_t* builder,
    const std::vector<std::filesystem::path>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const std::string& library_stem,
    library_type_t library_type
) {
    switch (library_type) {
        case LIBRARY_TYPE_STATIC: return create_static_library(builder, source_files, define_key_values, library_stem + ".a");
        case LIBRARY_TYPE_SHARED: return create_shared_library(builder, source_files, define_key_values, library_stem + ".so");
        default: throw std::runtime_error(std::format("create_library: unknown library_type {}", static_cast<int>(library_type)));
    }
}

std::filesystem::path cpp_compiler_t::create_binary(
    const builder_t* builder,
    const std::vector<std::filesystem::path>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    library_type_t library_type,
    const std::string& binary_name
) {
    return create_binary(
        builder->build_dir(library_type),
        builder->src_dir(),
        { builder->include_dir() },
        source_files,
        define_key_values,
        builder->export_libraries(library_type),
        library_type,
        builder->import_dir() / binary_name
    );
}

std::filesystem::path cpp_compiler_t::reference_static_library(
    const builder_t* builder,
    const std::filesystem::path& existing_static_library,
    const std::string& static_library_name
) {
    return reference_static_library(existing_static_library, builder->export_dir(LIBRARY_TYPE_STATIC) / static_library_name);
}

std::filesystem::path cpp_compiler_t::reference_shared_library(
    const builder_t* builder,
    const std::filesystem::path& existing_shared_library,
    const std::string& shared_library_name
) {
    return reference_shared_library(existing_shared_library, builder->export_dir(LIBRARY_TYPE_SHARED) / shared_library_name);
}

std::filesystem::path cpp_compiler_t::reference_library(
    const builder_t* builder,
    const std::filesystem::path& existing_library,
    const std::string& library_name,
    library_type_t library_type
) {
    switch (library_type) {
        case LIBRARY_TYPE_STATIC: return reference_static_library(builder, existing_library, library_name);
        case LIBRARY_TYPE_SHARED: return reference_shared_library(builder, existing_library, library_name);
        default: throw std::runtime_error(std::format("reference_library: unknown library_type {}", static_cast<int>(library_type)));
    }
}

std::filesystem::path cpp_compiler_t::reference_binary(
    const builder_t* builder,
    const std::filesystem::path& existing_binary,
    const std::string& binary_name
) {
    if (!std::filesystem::exists(existing_binary)) {
        throw std::runtime_error(std::format("reference_binary: referenced binary '{}' does not exist", existing_binary.string()));
    }

    const auto binary_dir = builder->import_dir();
    const auto binary = binary_dir / binary_name;

    std::cout << std::format("ln -s {} {}", existing_binary.string(), binary.string()) << std::endl;
    std::error_code ec;
    std::filesystem::create_symlink(existing_binary, binary, ec);
    if (ec) {
        throw std::runtime_error(std::format("reference_binary: failed to create symlink from '{}' to '{}': {}", existing_binary.string(), binary.string(), ec.message()));
    }

    return binary;
}

std::filesystem::path cpp_compiler_t::create_static_library(
    const std::filesystem::path& cache_dir,
    const std::filesystem::path& source_dir,
    const std::vector<std::filesystem::path>& include_dirs,
    const std::vector<std::filesystem::path>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const std::filesystem::path& static_library
) {
    const auto object_files = cache_object_files(
        cache_dir,
        source_dir,
        include_dirs,
        source_files,
        define_key_values,
        false
    );

    const auto static_library_dir = static_library.parent_path();
    if (!std::filesystem::exists(static_library_dir)) {
        if (!std::filesystem::create_directories(static_library_dir)) {
            throw std::runtime_error(std::format("create_static_library: failed to create output shared library directory '{}'", static_library_dir.string()));
        }
    }

    std::string command = "ar rcs " + static_library.string();
    for (const auto& object_file : object_files) {
        command += " " + object_file.string();
    }

    std::cout << command << std::endl;
    const int command_result = std::system(command.c_str());
    if (command_result != 0) {
        throw std::runtime_error(std::format("create_static_library: failed to create static library '{}', command exited with code {}", static_library.string(), command_result));
    }

    if (!std::filesystem::exists(static_library)) {
        throw std::runtime_error(std::format("create_static_library: expected output static library '{}' to exist but it does not", static_library.string()));
    }

    return static_library;
}

std::filesystem::path cpp_compiler_t::create_shared_library(
    const std::filesystem::path& cache_dir,
    const std::filesystem::path& source_dir,
    const std::vector<std::filesystem::path>& include_dirs,
    const std::vector<std::filesystem::path>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const std::vector<std::filesystem::path>& dsos,
    const std::filesystem::path& shared_library
) {
    const auto object_files = cache_object_files(
        cache_dir,
        source_dir,
        include_dirs,
        source_files,
        define_key_values,
        true
    );

    const auto shared_library_dir = shared_library.parent_path();
    if (!std::filesystem::exists(shared_library_dir)) {
        if (!std::filesystem::create_directories(shared_library_dir)) {
            throw std::runtime_error(std::format("create_shared_library: failed to create output shared library directory '{}'", shared_library_dir.string()));
        }
    }

    std::string command = "clang++ -shared -o " + shared_library.string();
    for (const auto& object_file : object_files) {
        command += " " + object_file.string();
    }

    for (const auto& dso : dsos) {
        if (!std::filesystem::exists(dso)) {
            throw std::runtime_error(std::format("create_shared_library: dso does not exist '{}'", dso.string()));
        }

        command += " " + dso.string();
    }

    std::cout << command << std::endl;
    const int command_result = std::system(command.c_str());
    if (command_result != 0) {
        throw std::runtime_error(std::format("create_shared_library: failed to create shared library '{}', command exited with code {}", shared_library.string(), command_result));
    }

    if (!std::filesystem::exists(shared_library)) {
        throw std::runtime_error(std::format("create_shared_library: expected output shared library '{}' to exist but it does not", shared_library.string()));
    }

    return shared_library;
}

std::filesystem::path cpp_compiler_t::create_binary(
    const std::filesystem::path& cache_dir,
    const std::filesystem::path& source_dir,
    const std::vector<std::filesystem::path>& include_dirs,
    const std::vector<std::filesystem::path>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const std::vector<std::vector<std::filesystem::path>>& library_groups,
    library_type_t library_type,
    const std::filesystem::path& binary
) {
    const auto object_files = cache_object_files(cache_dir, source_dir, include_dirs, source_files, define_key_values, library_type);

    const auto binary_dir = binary.parent_path();
    if (!std::filesystem::exists(binary_dir)) {
        if (!std::filesystem::create_directories(binary_dir)) {
            throw std::runtime_error(std::format("create_binary: failed to create output binary directory '{}'", binary_dir.string()));
        }
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
            if (!std::filesystem::exists(library)) {
                throw std::runtime_error(std::format("create_binary: library does not exist '{}'", library.string()));
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
        throw std::runtime_error(std::format("create_binary: failed to create binary '{}', command exited with code {}", binary.string(), command_result));
    }

    if (!std::filesystem::exists(binary)) {
        throw std::runtime_error(std::format("create_binary: expected output binary '{}' to exist but it does not", binary.string()));
    }

    return binary;
}

std::filesystem::path cpp_compiler_t::reference_static_library(
    const std::filesystem::path& existing_static_library,
    const std::filesystem::path& static_library
) {
    if (!std::filesystem::exists(existing_static_library)) {
        throw std::runtime_error(std::format("reference_static_library: referenced static library '{}' does not exist", existing_static_library.string()));
    }

    const auto static_library_dir = static_library.parent_path();
    if (!std::filesystem::exists(static_library_dir)) {
        if (!std::filesystem::create_directories(static_library_dir)) {
            throw std::runtime_error(std::format("reference_static_library: failed to create output static library directory '{}'", static_library_dir.string()));
        }
    }

    std::cout << std::format("ln -s {} {}", existing_static_library.string(), static_library.string()) << std::endl;
    std::error_code ec;
    std::filesystem::create_symlink(existing_static_library, static_library, ec);
    if (ec) {
        throw std::runtime_error(std::format("reference_static_library: failed to create symlink from '{}' to '{}': {}", existing_static_library.string(), static_library.string(), ec.message()));
    }

    return static_library;
}

std::filesystem::path cpp_compiler_t::reference_shared_library(
    const std::filesystem::path& existing_shared_library,
    const std::filesystem::path& shared_library
) {
    if (!std::filesystem::exists(existing_shared_library)) {
        throw std::runtime_error(std::format("reference_shared_library: referenced shared library '{}' does not exist", existing_shared_library.string()));
    }

    const auto shared_library_dir = shared_library.parent_path();
    if (!std::filesystem::exists(shared_library_dir)) {
        if (!std::filesystem::create_directories(shared_library_dir)) {
            throw std::runtime_error(std::format("reference_shared_library: failed to create output shared library directory '{}'", shared_library_dir.string()));
        }
    }

    std::cout << std::format("ln -s {} {}", existing_shared_library.string(), shared_library.string()) << std::endl;
    std::error_code ec;
    std::filesystem::create_symlink(existing_shared_library, shared_library, ec);
    if (ec) {
        throw std::runtime_error(std::format("reference_shared_library: failed to create symlink from '{}' to '{}': {}", existing_shared_library.string(), shared_library.string(), ec.message()));
    }

    return shared_library;
}

std::filesystem::path cpp_compiler_t::reference_shared_library(
    const std::filesystem::path& existing_library,
    const std::filesystem::path& library_stem,
    library_type_t library_type
) {
    switch (library_type) {
        case LIBRARY_TYPE_STATIC: return reference_static_library(existing_library, library_stem.string() + ".a");
        case LIBRARY_TYPE_SHARED: return reference_shared_library(existing_library, library_stem.string() + ".so");
        default: throw std::runtime_error(std::format("reference_library: unknown library_type {}", static_cast<int>(library_type)));
    }
}

std::vector<std::filesystem::path> cpp_compiler_t::cache_object_files(
    const builder_t* builder,
    const std::vector<std::filesystem::path>& source_files,
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

std::vector<std::filesystem::path> cpp_compiler_t::cache_object_files(
    const std::filesystem::path& cache_dir,
    const std::filesystem::path& source_dir,
    const std::vector<std::filesystem::path>& include_dirs,
    const std::vector<std::filesystem::path>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    bool position_independent
) {
    std::vector<std::filesystem::path> result;
    result.reserve(source_files.size());

    if (!std::filesystem::exists(cache_dir)) {
        if (!std::filesystem::create_directories(cache_dir)) {
            throw std::runtime_error(std::format("cache_object_files: failed to create output directory '{}'", cache_dir.string()));
        }
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
        if (!std::filesystem::exists(source_file)) {
            throw std::runtime_error(std::format("cache_object_files: source file does not exist '{}'", source_file.string()));
        }

        const auto rel = source_file.lexically_relative(source_dir);
        if (rel.empty() || rel.native().starts_with("..")) {
            throw std::runtime_error(std::format("cache_object_files: source file '{}' is not relative to source dir '{}'", source_file.string(), source_dir.string()));
        }

        auto object_file = cache_dir / rel;
        object_file.replace_extension(".o");

        const auto object_file_dir = object_file.parent_path();
        if (!std::filesystem::exists(object_file_dir)) {
            if (!std::filesystem::create_directories(object_file_dir)) {
                throw std::runtime_error(std::format("cache_object_files: failed to create output object file directory '{}'", object_file_dir.string()));
            }
        }

        std::string compile_command = std::format("clang++ {}{}-g {}-std=c++23 -c {} -o {}", include_dirs_str, define_flags_str, position_independent ? "-fPIC " : "", source_file.string(), object_file.string());
        std::cout << compile_command << std::endl;
        const int compile_command_result = std::system(compile_command.c_str());
        if (compile_command_result != 0) {
            throw std::runtime_error(std::format("cache_object_files: failed to compile '{}', compilation result: {}", source_file.string(), compile_command_result));
        }

        result.push_back(object_file);
    }

    return result;
}
