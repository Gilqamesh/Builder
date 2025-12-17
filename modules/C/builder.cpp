#include <modules/builder/builder.h>
#include <modules/builder/build.h>

BUILDER_EXTERN void builder__build_self(builder_ctx_t* ctx, const builder_api_t* api) {
    build_t::lib(ctx, api, {"c.cpp"});
}

BUILDER_EXTERN void builder__build_module(builder_ctx_t* ctx, const builder_api_t* api, const char* static_libs) {
    build_t::binary(ctx, api, "main.cpp", "c", static_libs);
}
