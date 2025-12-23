#include "builder.h"
#include "builder_plugin_internal.h"
#include "builder_api.h"

#include <format>
#include <cstring>

std::filesystem::path builder_t::materialize_static_library(
    builder_ctx_t* ctx, const builder_api_t* api, const std::string& static_library_name,
    const std::vector<std::filesystem::path>& cpp_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values
) {
    const auto static_bundle_dir = api->static_bundle_dir(ctx);

    std::vector<std::filesystem::path> objects = materialize_object_files(
        static_bundle_dir / ARTIFACT_CACHE_DIR,
        api->modules_dir(ctx),
        api->source_dir(ctx),
        cpp_files,
        define_key_values,
        false // NOTE: currently PIC path isn't handled
    );

    return compiler_t::update_static_library(objects, static_bundle_dir / static_library_name);
}

std::filesystem::path builder_t::reference_static_library(builder_ctx_t* ctx, const builder_api_t* api, const std::filesystem::path& static_library_path) {
    if (!std::filesystem::exists(static_library_path)) {
        throw std::runtime_error(std::format("referenced static library '{}' does not exist", static_library_path.string()));
    }

    std::filesystem::path install_path = api->static_bundle_dir(ctx) / static_library_path.filename();

    std::filesystem::create_symlink(static_library_path, install_path);

    return install_path;
}

std::filesystem::path builder_t::materialize_shared_library(
    builder_ctx_t* ctx, const builder_api_t* api, const std::string& shared_library_name,
    const std::vector<std::filesystem::path>& cpp_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values
) {
    const auto shared_bundle_dir = api->shared_bundle_dir(ctx);

    std::vector<std::filesystem::path> objects = materialize_object_files(
        shared_bundle_dir / ARTIFACT_CACHE_DIR,
        api->modules_dir(ctx),
        api->source_dir(ctx),
        cpp_files,
        define_key_values,
        true
    );

    return compiler_t::update_shared_libary(objects, shared_bundle_dir / shared_library_name);
}

std::filesystem::path builder_t::reference_shared_library(builder_ctx_t* ctx, const builder_api_t* api, const std::filesystem::path& shared_library_path) {
    if (!std::filesystem::exists(shared_library_path)) {
        throw std::runtime_error(std::format("referenced shared library '{}' does not exist", shared_library_path.string()));
    }

    std::filesystem::path install_path = api->shared_bundle_dir(ctx) / shared_library_path.filename();

    std::filesystem::create_symlink(shared_library_path, install_path);

    return install_path;
}

std::filesystem::path builder_t::materialize_binary(
    builder_ctx_t* ctx, const builder_api_t* api, const std::string& binary_name,
    const std::vector<std::filesystem::path>& cpp_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    std::vector<compiler_t::binary_file_input_t> binary_inputs
) {
    const auto linked_artifacts_dir = api->linked_artifacts_dir(ctx);
    std::vector<std::filesystem::path> objects = materialize_object_files(
        linked_artifacts_dir / ARTIFACT_CACHE_DIR,
        api->modules_dir(ctx),
        api->source_dir(ctx),
        cpp_files,
        define_key_values,
        false
    );

    binary_inputs.insert(binary_inputs.begin(), objects);

    return compiler_t::update_binary(binary_inputs, {}, linked_artifacts_dir / binary_name);
}

std::filesystem::path builder_t::reference_binary(builder_ctx_t* ctx, const builder_api_t* api, const std::filesystem::path& binary_path) {
    if (!std::filesystem::exists(binary_path)) {
        throw std::runtime_error(std::format("referenced binary '{}' does not exist", binary_path.string()));
    }

    std::filesystem::path install_path = api->linked_artifacts_dir(ctx) / binary_path.filename();

    std::filesystem::create_symlink(binary_path, install_path);

    return binary_path;
}

std::vector<std::filesystem::path> builder_t::materialize_object_files(
    const std::filesystem::path& install_dir,
    const std::filesystem::path& modules_dir,
    const std::filesystem::path& source_dir,
    const std::vector<std::filesystem::path>& cpp_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    bool position_independent
) {
    std::vector<std::filesystem::path> result;
    result.reserve(cpp_files.size());

    const auto source_dir_id = source_dir.filename();

    for (const auto& cpp_file : cpp_files) {
        const auto src_file = cpp_file.is_absolute() ? cpp_file : source_dir / cpp_file;
        const auto rel = src_file.lexically_relative(source_dir);
        if (rel.empty() || rel.string().starts_with("..")) {
            throw std::runtime_error(std::format("cpp file '{}' is not under src dir '{}'", cpp_file.string(), source_dir.string()));
        }

        auto obj_path = install_dir / source_dir_id / rel;
        obj_path.replace_extension(".o");

        const auto obj = compiler_t::update_object_file(
            src_file,
            { modules_dir.parent_path() },
            define_key_values,
            obj_path,
            position_independent
        );
        result.push_back(obj);
    }

    return result;
}
