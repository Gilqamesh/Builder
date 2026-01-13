#include <builder/compiler/cpp_compiler.h>
#include <builder/process/process.h>

#include <format>
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
    const auto exported_interfaces = builder->export_interfaces(library_type_t::STATIC);
    return create_static_library(
        builder->libraries_build_dir(library_type_t::STATIC),
        builder->source_dir(),
        exported_interfaces,
        source_files,
        define_key_values,
        builder->libraries_build_dir(library_type_t::STATIC) / relative_static_library_path
    );
}

path_t cpp_compiler_t::create_shared_library(
    const builder_t* builder,
    const std::vector<path_t>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const relative_path_t& relative_shared_library_path
) {
    const auto exported_interfaces = builder->export_interfaces(library_type_t::SHARED);
    return create_shared_library(
        builder->libraries_build_dir(library_type_t::SHARED),
        builder->source_dir(),
        exported_interfaces,
        source_files,
        define_key_values,
        {},
        builder->libraries_build_dir(library_type_t::SHARED) / relative_shared_library_path
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
        case library_type_t::STATIC: return create_static_library(builder, source_files, define_key_values, relative_libary_stem + ".a");
        case library_type_t::SHARED: return create_shared_library(builder, source_files, define_key_values, relative_libary_stem + ".so");
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
    const auto exported_interfaces = builder->export_interfaces(library_type);
    const auto exported_libraries = builder->export_libraries(library_type);

    return create_binary(
        builder->import_build_dir(),
        builder->source_dir(),
        exported_interfaces,
        source_files,
        define_key_values,
        exported_libraries,
        library_type,
        builder->import_build_dir() / relative_binary_path
    );
}

path_t cpp_compiler_t::reference_static_library(
    const builder_t* builder,
    const path_t& existing_static_library,
    const relative_path_t& relative_static_library_path
) {
    return reference_static_library(existing_static_library, builder->libraries_build_dir(library_type_t::STATIC) / relative_static_library_path);
}

path_t cpp_compiler_t::reference_shared_library(
    const builder_t* builder,
    const path_t& existing_shared_library,
    const relative_path_t& relative_shared_library_path
) {
    return reference_shared_library(existing_shared_library, builder->libraries_build_dir(library_type_t::SHARED) / relative_shared_library_path);
}

path_t cpp_compiler_t::reference_library(
    const builder_t* builder,
    const path_t& existing_library,
    const relative_path_t& relative_library_path,
    library_type_t library_type
) {
    switch (library_type) {
        case library_type_t::STATIC: return reference_static_library(builder, existing_library, relative_library_path);
        case library_type_t::SHARED: return reference_shared_library(builder, existing_library, relative_library_path);
        default: throw std::runtime_error(std::format("cpp_compiler_t::reference_library: unknown library_type {}", static_cast<int>(library_type)));
    }
}

path_t cpp_compiler_t::reference_binary(
    const builder_t* builder,
    const path_t& existing_binary,
    const relative_path_t& relative_binary_path
) {
    if (!filesystem_t::exists(existing_binary)) {
        throw std::runtime_error(std::format("cpp_compiler_t::reference_binary: referenced binary '{}' does not exist", existing_binary));
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

    std::vector<process_arg_t> process_args;
    process_args.push_back(AR_PATH);
    process_args.push_back("rcs");
    process_args.push_back(static_library);
    for (const auto& object_file : object_files) {
        process_args.push_back(object_file);
    }

    const int process_result = process_t::create_and_wait(process_args);
    if (process_result != 0) {
        throw std::runtime_error(std::format("cpp_compiler_t::create_static_library: failed to create static library '{}', command exited with code {}", static_library, process_result));
    }

    if (!filesystem_t::exists(static_library)) {
        throw std::runtime_error(std::format("cpp_compiler_t::create_static_library: expected output static library '{}' to exist but it does not", static_library));
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

    std::vector<process_arg_t> process_args;
    process_args.push_back(CPP_COMPILER_PATH);
    process_args.push_back("-g");
    process_args.push_back("-shared");
    process_args.push_back("-o");
    process_args.push_back(shared_library);
    for (const auto& object_file : object_files) {
        process_args.push_back(object_file);
    }

    for (const auto& dso : dsos) {
        if (!filesystem_t::exists(dso)) {
            throw std::runtime_error(std::format("cpp_compiler_t::create_shared_library: dso does not exist '{}'", dso));
        }

        process_args.push_back(dso);
    }

    const int process_result = process_t::create_and_wait(process_args);
    if (process_result != 0) {
        throw std::runtime_error(std::format("cpp_compiler_t::create_shared_library: failed to create shared library '{}', command exited with code {}", shared_library, process_result));
    }

    if (!filesystem_t::exists(shared_library)) {
        throw std::runtime_error(std::format("cpp_compiler_t::create_shared_library: expected output shared library '{}' to exist but it does not", shared_library));
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
    const auto object_files = cache_object_files(cache_dir, source_dir, include_dirs, source_files, define_key_values, library_type != library_type_t::STATIC);

    const auto binary_dir = binary.parent();
    if (!filesystem_t::exists(binary_dir)) {
        filesystem_t::create_directories(binary_dir);
    }

    std::vector<process_arg_t> process_args;
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
        if (library_type != library_type_t::SHARED && 1 < library_group_it->size()) {
            process_args.push_back("-Wl,--start-group");
        }
        for (const auto& library : *library_group_it) {
            if (!filesystem_t::exists(library)) {
                throw std::runtime_error(std::format("cpp_compiler_t::create_binary: library does not exist '{}'", library));
            }
            process_args.push_back(library);
        }
        if (library_type != library_type_t::SHARED && 1 < library_group_it->size()) {
            process_args.push_back("-Wl,--end-group");
        }
    }

    for (const auto& library_group : library_groups) {
        for (const auto& library_group : library_group) {
            process_args.push_back(std::format("-Wl,-rpath,{}", library_group.parent()));
        }
    }
    
    const int process_result = process_t::create_and_wait(process_args);
    if (process_result != 0) {
        throw std::runtime_error(std::format("cpp_compiler_t::create_binary: failed to create binary '{}', command exited with code {}", binary, process_result));
    }

    if (!filesystem_t::exists(binary)) {
        throw std::runtime_error(std::format("cpp_compiler_t::create_binary: expected output binary '{}' to exist but it does not", binary));
    }

    return binary;
}

path_t cpp_compiler_t::reference_static_library(
    const path_t& existing_static_library,
    const path_t& static_library
) {
    if (!filesystem_t::exists(existing_static_library)) {
        throw std::runtime_error(std::format("cpp_compiler_t::reference_static_library: referenced static library '{}' does not exist", existing_static_library));
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
        throw std::runtime_error(std::format("cpp_compiler_t::reference_shared_library: referenced shared library '{}' does not exist", existing_shared_library));
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
        case library_type_t::STATIC: return reference_static_library(existing_library, library_stem + ".a");
        case library_type_t::SHARED: return reference_shared_library(existing_library, library_stem + ".so");
        default: throw std::runtime_error(std::format("cpp_compiler_t::reference_shared_library: unknown library_type {}", static_cast<int>(library_type)));
    }
}

std::vector<path_t> cpp_compiler_t::cache_object_files(
    const builder_t* builder,
    const std::vector<path_t>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    library_type_t library_type
) {
    const auto exported_interfaces = builder->export_interfaces(library_type);
    return cache_object_files(
        builder->libraries_build_dir(library_type),
        builder->source_dir(),
        exported_interfaces,
        source_files,
        define_key_values,
        library_type == library_type_t::SHARED
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

    std::vector<process_arg_t> process_prefix_args;
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
        if (!filesystem_t::exists(source_file)) {
            throw std::runtime_error(std::format("cpp_compiler_t::cache_object_files: source file does not exist '{}'", source_file));
        }

        const auto rel = source_dir.relative(source_file);
        auto object_file = cache_dir / rel;
        object_file.extension(".o");

        const auto object_file_dir = object_file.parent();
        if (!filesystem_t::exists(object_file_dir)) {
            filesystem_t::create_directories(object_file_dir);
        }

        std::vector<process_arg_t> process_args = process_prefix_args;
        if (position_independent) {
            process_args.push_back("-fPIC");
        }
        process_args.push_back("-c");
        process_args.push_back(source_file);
        process_args.push_back("-o");
        process_args.push_back(object_file);

        const int process_result = process_t::create_and_wait(process_args);
        if (process_result != 0) {
            throw std::runtime_error(std::format("cpp_compiler_t::cache_object_files: failed to compile '{}', compilation result: {}", source_file, process_result));
        }

        result.push_back(object_file);
    }

    return result;
}
