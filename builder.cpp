#include "builder.h"
#include "builder_plugin_internal.h"

std::filesystem::path builder_t::lib(
    builder_ctx_t* ctx,
    const builder_api_t* api,
    const std::vector<std::string>& cpp_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    bool position_independent
) {
    const auto modules_dir = std::filesystem::path(api->modules_dir(ctx));
    const auto artifact_dir = std::filesystem::path(api->artifact_dir(ctx));
    const auto src_dir = std::filesystem::path(api->src_dir(ctx));

    std::vector<std::filesystem::path> objects;
    for (const auto& cpp_file : cpp_files) {
        const auto obj = compiler_t::update_object_file(
            src_dir / cpp_file,
            {},
            { modules_dir.parent_path() },
            define_key_values,
            artifact_dir / (std::filesystem::path(cpp_file).stem().string() + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".o"),
            position_independent
        );
        objects.push_back(obj);
    }

    return compiler_t::update_static_library(objects, artifact_dir / (position_independent ? API_STATIC_PIC_LIB_NAME : API_STATIC_LIB_NAME));
}

std::filesystem::path builder_t::so(
    builder_ctx_t* ctx,
    const builder_api_t* api,
    const std::vector<std::string>& cpp_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values
) {
    const auto modules_dir = std::filesystem::path(api->modules_dir(ctx));
    const auto artifact_dir = std::filesystem::path(api->artifact_dir(ctx));
    const auto src_dir = std::filesystem::path(api->src_dir(ctx));

    std::vector<std::filesystem::path> objects;
    for (const auto& cpp_file : cpp_files) {
        const auto obj = compiler_t::update_object_file(
            src_dir / cpp_file,
            {},
            { modules_dir.parent_path() },
            define_key_values,
            artifact_dir / (std::filesystem::path(cpp_file).stem().string() + ".o"),
            true
        );
        objects.push_back(obj);
    }

    return compiler_t::update_shared_libary(objects, artifact_dir / API_SHARED_LIB_NAME);
}

std::filesystem::path builder_t::binary(
    builder_ctx_t* ctx,
    const builder_api_t* api,
    const std::vector<std::string>& cpp_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const std::string& bin_name,
    std::vector<compiler_t::binary_input_t> binary_inputs
) {
    const auto modules_dir = std::filesystem::path(api->modules_dir(ctx));
    const auto artifact_dir = std::filesystem::path(api->artifact_dir(ctx));
    const auto src_dir = std::filesystem::path(api->src_dir(ctx));

    std::vector<std::filesystem::path> objects;
    for (const auto& cpp_file : cpp_files) {
        const auto obj = compiler_t::update_object_file(
            src_dir / cpp_file,
            {},
            { modules_dir.parent_path() },
            define_key_values,
            artifact_dir / (std::filesystem::path(cpp_file).stem().string() + ".o"),
            true // TODO: this should be false
        );
        objects.push_back(obj);
    }

    binary_inputs.insert(binary_inputs.begin(), objects);

    return compiler_t::update_binary(binary_inputs, artifact_dir / bin_name);
}
