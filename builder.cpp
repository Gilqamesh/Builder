#include "builder.h"
#include "builder_plugin_internal.h"

#include <format>

std::vector<std::filesystem::path> builder_t::cpp_files_to_objects(
    const std::filesystem::path& artifact_dir,
    const std::filesystem::path& modules_dir,
    const std::filesystem::path& src_dir,
    const std::vector<std::filesystem::path>& cpp_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    bool position_independent
) {
    std::vector<std::filesystem::path> result;
    result.reserve(cpp_files.size());

    const auto src_dir_id = src_dir.filename();

    for (const auto& cpp_file : cpp_files) {
        const auto src_file = cpp_file.is_absolute() ? cpp_file : src_dir / cpp_file;
        const auto rel = src_file.lexically_relative(src_dir);
        if (rel.empty() || rel.string().starts_with("..")) {
            throw std::runtime_error(std::format("cpp file '{}' is not under src dir '{}'", cpp_file.string(), src_dir.string()));
        }

        auto obj_path = artifact_dir / src_dir_id / rel;
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

std::filesystem::path builder_t::lib(
    builder_ctx_t* ctx,
    const builder_api_t* api,
    const std::vector<std::filesystem::path>& cpp_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    bool position_independent
) {
    const auto modules_dir = std::filesystem::path(api->modules_dir(ctx));
    const auto artifact_dir = std::filesystem::path(api->artifact_dir(ctx));
    const auto src_dir = std::filesystem::path(api->src_dir(ctx));

    std::vector<std::filesystem::path> objects = cpp_files_to_objects(
        artifact_dir,
        modules_dir,
        src_dir,
        cpp_files,
        define_key_values,
        position_independent
    );

    return compiler_t::update_static_library(objects, artifact_dir / (position_independent ? API_STATIC_PIC_LIB_NAME : API_STATIC_LIB_NAME));
}

std::filesystem::path builder_t::so(
    builder_ctx_t* ctx,
    const builder_api_t* api,
    const std::vector<std::filesystem::path>& cpp_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values
) {
    const auto modules_dir = std::filesystem::path(api->modules_dir(ctx));
    const auto artifact_dir = std::filesystem::path(api->artifact_dir(ctx));
    const auto src_dir = std::filesystem::path(api->src_dir(ctx));

    std::vector<std::filesystem::path> objects = cpp_files_to_objects(
        artifact_dir,
        modules_dir,
        src_dir,
        cpp_files,
        define_key_values,
        true
    );

    return compiler_t::update_shared_libary(objects, artifact_dir / API_SHARED_LIB_NAME);
}

std::filesystem::path builder_t::binary(
    builder_ctx_t* ctx,
    const builder_api_t* api,
    const std::vector<std::filesystem::path>& cpp_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const std::string& bin_name,
    std::vector<compiler_t::binary_file_input_t> binary_inputs,
    const std::vector<std::string>& additional_linker_flags
) {
    const auto modules_dir = std::filesystem::path(api->modules_dir(ctx));
    const auto artifact_dir = std::filesystem::path(api->artifact_dir(ctx));
    const auto src_dir = std::filesystem::path(api->src_dir(ctx));

    std::vector<std::filesystem::path> objects = cpp_files_to_objects(
        artifact_dir,
        modules_dir,
        src_dir,
        cpp_files,
        define_key_values,
        true // TODO: this should be false
    );

    binary_inputs.insert(binary_inputs.begin(), objects);

    return compiler_t::update_binary(binary_inputs, additional_linker_flags, artifact_dir / bin_name);
}
