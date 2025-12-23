#include "builder.h"
#include "builder_plugin.h"
#include "builder_plugin_internal.h"
#include "builder_api.h"

#include <filesystem>

static std::vector<std::filesystem::path> collect_library_cpp_files(builder_ctx_t* ctx, const builder_api_t* api) {
    std::vector<std::filesystem::path> result;

    for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::path(api->source_dir(ctx)))) {
        const auto& path = entry.path();
        const auto& filename = path.filename().string();
        if (path.extension() != ".cpp" || filename == BUILDER_DRIVER_CPP || filename == BUILDER_PLUGIN_CPP) {
            continue ;
        }
        result.push_back(path);
    }

    return result;
}

BUILDER_EXTERN void builder__export_bundle_static(builder_ctx_t* ctx, const builder_api_t* api) {
    builder_t::materialize_static_library(ctx, api, "1.lib", collect_library_cpp_files(ctx, api), {});
}

BUILDER_EXTERN void builder__export_bundle_shared(builder_ctx_t* ctx, const builder_api_t* api) {
    builder_t::materialize_shared_library(ctx, api, "1.so", collect_library_cpp_files(ctx, api), {});
}

BUILDER_EXTERN void builder__link_module(builder_ctx_t* ctx, const builder_api_t* api) {
    builder_t::materialize_binary(
        ctx, api, BUILDER_DRIVER,
        { BUILDER_DRIVER_CPP },
        { { "VERSION", std::to_string(api->version) } },
        { api->get_shared_link_command_line(ctx) }
    );
}
