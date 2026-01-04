#include <builder/compiler/cpp_compiler.h>

#include <format>
#include <iostream>
#include <ranges>
#include <chrono>
#include <sstream>
#include <fstream>

std::filesystem::path cpp_compiler_t::create_static_library(
    builder_ctx_t* ctx, const builder_api_t* api,
    const std::vector<std::filesystem::path>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const std::string& static_library_name
) {
    return create_static_library(
        api->cache_dir(ctx, BUNDLE_TYPE_STATIC),
        api->source_dir(ctx),
        { api->include_dir(ctx) },
        source_files,
        define_key_values,
        api->install_dir(ctx, BUNDLE_TYPE_STATIC) / static_library_name
    );
}

std::filesystem::path cpp_compiler_t::create_shared_library(
    builder_ctx_t* ctx, const builder_api_t* api,
    const std::vector<std::filesystem::path>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const std::string& shared_library_name
) {
    return create_shared_library(
        api->cache_dir(ctx, BUNDLE_TYPE_SHARED),
        api->source_dir(ctx),
        { api->include_dir(ctx) },
        source_files,
        define_key_values,
        api->install_dir(ctx, BUNDLE_TYPE_SHARED) / shared_library_name
    );
}

std::filesystem::path cpp_compiler_t::create_library(
    builder_ctx_t* ctx, const builder_api_t* api,
    const std::vector<std::filesystem::path>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const std::string& library_stem,
    bundle_type_t bundle_type
) {
    switch (bundle_type) {
        case BUNDLE_TYPE_STATIC: return create_static_library(ctx, api, source_files, define_key_values, library_stem + ".lib");
        case BUNDLE_TYPE_SHARED: return create_shared_library(ctx, api, source_files, define_key_values, library_stem + ".so");
        default: throw std::runtime_error(std::format("create_library: unknown bundle_type {}", static_cast<int>(bundle_type)));
    }
}

std::filesystem::path cpp_compiler_t::create_loadable(
    builder_ctx_t* ctx, const builder_api_t* api,
    const std::vector<std::filesystem::path>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    bundle_type_t bundle_type,
    const std::string& loadable_name
) {
    return create_loadable(
        api->cache_dir(ctx, bundle_type),
        api->source_dir(ctx),
        { api->include_dir(ctx) },
        source_files,
        define_key_values,
        api->export_libraries(ctx, bundle_type),
        bundle_type,
        api->build_module_dir(ctx) / loadable_name
    );
}

std::filesystem::path cpp_compiler_t::reference_static_library(
    builder_ctx_t* ctx, const builder_api_t* api,
    const std::filesystem::path& existing_static_library,
    const std::string& static_library_name
) {
    return reference_static_library(existing_static_library, api->install_dir(ctx, BUNDLE_TYPE_STATIC) / static_library_name);
}

std::filesystem::path cpp_compiler_t::reference_shared_library(
    builder_ctx_t* ctx, const builder_api_t* api,
    const std::filesystem::path& existing_shared_library,
    const std::string& shared_library_name
) {
    return reference_shared_library(existing_shared_library, api->install_dir(ctx, BUNDLE_TYPE_SHARED) / shared_library_name);
}

std::filesystem::path cpp_compiler_t::reference_library(
    builder_ctx_t* ctx, const builder_api_t* api,
    const std::filesystem::path& existing_library,
    const std::string& library_name,
    bundle_type_t bundle_type
) {
    switch (bundle_type) {
        case BUNDLE_TYPE_STATIC: return reference_static_library(ctx, api, existing_library, library_name);
        case BUNDLE_TYPE_SHARED: return reference_shared_library(ctx, api, existing_library, library_name);
        default: throw std::runtime_error(std::format("reference_library: unknown bundle_type {}", static_cast<int>(bundle_type)));
    }
}

std::filesystem::path cpp_compiler_t::reference_loadable(
    builder_ctx_t* ctx, const builder_api_t* api,
    const std::filesystem::path& existing_loadable,
    const std::string& loadable_name
) {
    if (!std::filesystem::exists(existing_loadable)) {
        throw std::runtime_error(std::format("reference_loadable: referenced loadable '{}' does not exist", existing_loadable.string()));
    }

    const auto loadable_dir = api->build_module_dir(ctx);
    const auto loadable = loadable_dir / loadable_name;

    std::cout << std::format("ln -s {} {}", existing_loadable.string(), loadable.string()) << std::endl;
    std::error_code ec;
    std::filesystem::create_symlink(existing_loadable, loadable, ec);
    if (ec) {
        throw std::runtime_error(std::format("reference_loadable: failed to create symlink from '{}' to '{}': {}", existing_loadable.string(), loadable.string(), ec.message()));
    }

    return loadable;
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

std::filesystem::path cpp_compiler_t::create_library(
    const std::filesystem::path& cache_dir,
    const std::filesystem::path& source_dir,
    const std::vector<std::filesystem::path>& include_dirs,
    const std::vector<std::filesystem::path>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const std::filesystem::path& library_stem,
    bundle_type_t bundle_type
) {
    switch (bundle_type) {
        case BUNDLE_TYPE_STATIC: return create_static_library(cache_dir, source_dir, include_dirs, source_files, define_key_values, library_stem.string() + ".lib");
        case BUNDLE_TYPE_SHARED: return create_shared_library(cache_dir, source_dir, include_dirs, source_files, define_key_values, library_stem.string() + ".so");
        default: throw std::runtime_error(std::format("create_library: unknown bundle_type {}", static_cast<int>(bundle_type)));
    }
}

std::filesystem::path cpp_compiler_t::create_loadable(
    const std::filesystem::path& cache_dir,
    const std::filesystem::path& source_dir,
    const std::vector<std::filesystem::path>& include_dirs,
    const std::vector<std::filesystem::path>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const std::vector<std::vector<std::filesystem::path>>& library_groups,
    bundle_type_t bundle_type,
    const std::filesystem::path& loadable
) {
    const auto object_files = cache_object_files(cache_dir, source_dir, include_dirs, source_files, define_key_values, bundle_type);

    const auto loadable_dir = loadable.parent_path();
    if (!std::filesystem::exists(loadable_dir)) {
        if (!std::filesystem::create_directories(loadable_dir)) {
            throw std::runtime_error(std::format("create_loadable: failed to create output loadable directory '{}'", loadable_dir.string()));
        }
    }

    std::string command = std::format("clang++ -std=c++23 {}-o {}", bundle_type == BUNDLE_TYPE_SHARED ? "-shared " : "", loadable.string());
    for (const auto& object_file : object_files) {
        command += " " + object_file.string();
    }

    // TODO: mixed static/shared interleave of start/end group, typify 'library_groups'
    for (auto library_group_it = library_groups.rbegin(); library_group_it != library_groups.rend(); ++library_group_it) {
        if (bundle_type != BUNDLE_TYPE_SHARED && 1 < library_group_it->size()) {
            command += " -Wl,--start-group";
        }
        for (const auto& library : *library_group_it) {
            if (!std::filesystem::exists(library)) {
                throw std::runtime_error(std::format("create_loadable: library does not exist '{}'", library.string()));
            }
            command += " " + library.string();
        }
        if (bundle_type != BUNDLE_TYPE_SHARED && 1 < library_group_it->size()) {
            command += " -Wl,--end-group";
        }
    }

    std::cout << command << std::endl;
    const int command_result = std::system(command.c_str());
    if (command_result != 0) {
        throw std::runtime_error(std::format("create_loadable: failed to create loadable '{}', command exited with code {}", loadable.string(), command_result));
    }

    if (!std::filesystem::exists(loadable)) {
        throw std::runtime_error(std::format("create_loadable: expected output loadable '{}' to exist but it does not", loadable.string()));
    }

    return loadable;
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
    bundle_type_t bundle_type
) {
    switch (bundle_type) {
        case BUNDLE_TYPE_STATIC: return reference_static_library(existing_library, library_stem.string() + ".lib");
        case BUNDLE_TYPE_SHARED: return reference_shared_library(existing_library, library_stem.string() + ".so");
        default: throw std::runtime_error(std::format("reference_library: unknown bundle_type {}", static_cast<int>(bundle_type)));
    }
}

std::vector<std::filesystem::path> cpp_compiler_t::cache_object_files(
    builder_ctx_t* ctx, const builder_api_t* api,
    const std::vector<std::filesystem::path>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    bundle_type_t bundle_type
) {
    return cache_object_files(
        api->cache_dir(ctx, bundle_type),
        api->source_dir(ctx),
        { api->include_dir(ctx) },
        source_files,
        define_key_values,
        bundle_type == BUNDLE_TYPE_SHARED
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
