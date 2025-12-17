#include <modules/builder/build.h>
#include <modules/builder/compiler.h>

std::filesystem::path build_t::lib(builder_ctx_t* ctx, const builder_api_t* api, const std::vector<std::string>& cpp_files) {
    const auto root_dir = std::filesystem::path(api->root_dir(ctx));
    const auto artifact_dir = std::filesystem::path(api->artifact_dir(ctx));
    const auto module_dir = std::filesystem::path(api->module_dir(ctx));

    std::vector<std::filesystem::path> objects;
    for (const auto& cpp_file : cpp_files) {
        const auto obj = compiler_t::update_object_file(
            module_dir / cpp_file,
            {},
            { root_dir },
            {},
            artifact_dir / (std::filesystem::path(cpp_file).stem().string() + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".o"),
            false
        );
        objects.push_back(obj);
    }

    return compiler_t::update_static_library(objects, artifact_dir / API_LIB_NAME);
}

std::filesystem::path build_t::so(builder_ctx_t* ctx, const builder_api_t* api, const std::vector<std::string>& cpp_files) {
    const auto root_dir = std::filesystem::path(api->root_dir(ctx));
    const auto artifact_dir = std::filesystem::path(api->artifact_dir(ctx));
    const auto module_dir = std::filesystem::path(api->module_dir(ctx));

    std::vector<std::filesystem::path> objects;
    for (const auto& cpp_file : cpp_files) {
        const auto obj = compiler_t::update_object_file(
            module_dir / cpp_file,
            {},
            { root_dir },
            {},
            artifact_dir / (std::filesystem::path(cpp_file).stem().string() + ".o"),
            true
        );
        objects.push_back(obj);
    }

    return compiler_t::update_shared_libary(objects, artifact_dir / API_SO_NAME);
}

std::filesystem::path build_t::binary(builder_ctx_t* ctx, const builder_api_t* api, const std::string& main_cpp_file, const std::string& bin_name, const char* static_libs) {
    const auto root_dir = std::filesystem::path(api->root_dir(ctx));
    const auto artifact_dir = std::filesystem::path(api->artifact_dir(ctx));
    const auto module_dir = std::filesystem::path(api->module_dir(ctx));

    const auto main_obj = (
        compiler_t::update_object_file(
            module_dir / main_cpp_file,
            {},
            { root_dir },
            {},
            artifact_dir / (std::filesystem::path(bin_name).stem().string() + ".o"),
            false
        )
    );

    return compiler_t::update_binary({ main_obj, static_libs }, artifact_dir / bin_name);
}
