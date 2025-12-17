#include <modules/builder/builder.h>
#include <modules/builder/compiler.h>

BUILDER_EXTERN void builder__build_self(builder_ctx_t* ctx, const builder_api_t* api) {
    const auto root_dir = std::filesystem::path(api->root_dir(ctx));
    const auto artifact_dir = std::filesystem::path(api->artifact_dir(ctx));
    const auto module_dir = std::filesystem::path(api->module_dir(ctx));

    compiler_t::update_shared_libary(
        {
            compiler_t::update_object_file(
                module_dir / "b.cpp",
                {},
                { root_dir },
                {},
                artifact_dir / "b.o",
                true
            )
        },
        artifact_dir / API_SO_NAME
    );

    compiler_t::update_static_library(
        {
            compiler_t::update_object_file(
                module_dir / "b.cpp",
                {},
                { root_dir },
                {},
                artifact_dir / "b_static.o",
                false
            )
        },
        artifact_dir / API_LIB_NAME
    );
}

BUILDER_EXTERN void builder__build_module(builder_ctx_t* ctx, const builder_api_t* api, const char* static_libs) {
    const auto root_dir = std::filesystem::path(api->root_dir(ctx));
    const auto artifact_dir = std::filesystem::path(api->artifact_dir(ctx));
    const auto module_dir = std::filesystem::path(api->module_dir(ctx));

    const auto main_obj = (
        compiler_t::update_object_file(
            module_dir / "main.cpp",
            {},
            { root_dir },
            {},
            artifact_dir / "main.o",
            false
        )
    );

    compiler_t::update_binary({ main_obj, static_libs }, artifact_dir / "b_bin");
}
