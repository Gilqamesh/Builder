#include <modules/builder/builder.h>
#include <modules/builder/build.h>
#include <modules/builder/compiler.h>
#include <modules/builder/builder_internal.h>

#include <string>
#include <filesystem>
#include <format>
#include <chrono>

BUILDER_EXTERN void builder__build_self(builder_ctx_t* ctx, const builder_api_t* api) {
    std::vector<std::string> lib_cpp_files;
    for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::path(api->src_dir(ctx)))) {
        const auto& path = entry.path();
        const auto filename = path.filename().string();
        if (path.extension() != ".cpp" || filename == ORCHESTRATOR_CPP || filename == BUILDER_CPP) {
            continue ;
        }
        lib_cpp_files.push_back(filename);
    }
    build_t::lib(ctx, api, lib_cpp_files, {});
    build_t::so(ctx, api, lib_cpp_files, {});

    std::vector<std::string> all_cpp_files;
    for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::path(api->src_dir(ctx)))) {
        const auto& path = entry.path();
        const auto filename = path.filename().string();
        if (path.extension() != ".cpp") {
            continue ;
        }
        all_cpp_files.push_back(filename);
    }
    build_t::binary(ctx, api, all_cpp_files, { { "VERSION", std::to_string(VERSION) } }, ORCHESTRATOR_BIN, { });
}

BUILDER_EXTERN void builder__build_module(builder_ctx_t* ctx, const builder_api_t* api, const char* static_libs) {
}
