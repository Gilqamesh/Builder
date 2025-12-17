#include <modules/builder/builder.h>
#include <modules/builder/compiler.h>
#include <modules/builder/builder_internal.h>

#include <string>
#include <filesystem>
#include <format>
#include <chrono>

BUILDER_EXTERN void builder__build_self(builder_ctx_t* ctx, const builder_api_t* api) {
    const auto root_dir = std::filesystem::path(api->root_dir(ctx));
    const auto artifact_dir = std::filesystem::path(api->artifact_dir(ctx));
    const auto module_dir = std::filesystem::path(api->module_dir(ctx));

    if (!std::filesystem::exists(module_dir)) {
        throw std::runtime_error(std::format("module directory does not exist '{}'", module_dir.string()));
    }

    const auto builder_obj = compiler_t::update_object_file(
        std::filesystem::path(module_dir) / BUILDER_CPP,
        {},
        { root_dir },
        {},
        artifact_dir / (BUILDER + std::string(".o")),
        true
    );

    const auto lib_static_objs = [&]() {
        std::vector<std::filesystem::path> objs;
        for (const auto& entry : std::filesystem::directory_iterator(module_dir)) {
            const auto& path = entry.path();
            const auto filename = path.filename().string();
            if (path.extension() != ".cpp" || filename == ORCHESTRATOR_CPP || filename == BUILDER_CPP) {
                continue ;
            }

            const auto stem = path.stem().string();
            objs.push_back(
                compiler_t::update_object_file(
                    path,
                    {},
                    { root_dir },
                    {},
                    artifact_dir / (stem + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".o"),
                    false
                )
            );
        }
        return objs;
    }();

    const auto lib_shared_objs = [&]() {
        std::vector<std::filesystem::path> objs;
        for (const auto& entry : std::filesystem::directory_iterator(module_dir)) {
            const auto& path = entry.path();
            const auto filename = path.filename().string();
            if (path.extension() != ".cpp" || filename == ORCHESTRATOR_CPP || filename == BUILDER_CPP) {
                continue ;
            }

            const auto stem = path.stem().string();
            objs.push_back(
                compiler_t::update_object_file(
                    path,
                    {},
                    { root_dir },
                    {},
                    artifact_dir / (stem + ".o"),
                    true
                )
            );
        }
        return objs;
    }();

    const auto so = compiler_t::update_shared_libary(lib_shared_objs, artifact_dir / API_SO_NAME);
    const auto lib = compiler_t::update_static_library(lib_static_objs, artifact_dir / API_LIB_NAME);

    compiler_t::update_shared_libary({ builder_obj, so }, artifact_dir / BUILDER_SO);

    const auto orchestrator_obj = compiler_t::update_object_file(
        module_dir / ORCHESTRATOR_CPP,
        { },
        { root_dir },
        { { "VERSION", std::to_string(VERSION) } },
        artifact_dir / (ORCHESTRATOR_BIN + std::string(".o")),
        false
    );

    const auto orchestrator_bin = compiler_t::update_binary({ orchestrator_obj, builder_obj, lib }, artifact_dir / ORCHESTRATOR_BIN);
}

BUILDER_EXTERN void builder__build_module(builder_ctx_t* ctx, const builder_api_t* api, const char* static_libs) {
}
