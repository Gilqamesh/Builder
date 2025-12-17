#ifndef MODULES_BUILDER_BUILDER_H
# define MODULES_BUILDER_BUILDER_H

# ifdef __cplusplus
#  define BUILDER_EXTERN extern "C"
# else
#  define BUILDER_EXTERN
# endif

# include <stdint.h>

# define API_LIB_NAME "api.lib"
# define API_SO_NAME  "api.so"

typedef struct builder_ctx_t builder_ctx_t;

typedef struct builder_api_t {
    uint64_t version;
    const char* (*root_dir)(builder_ctx_t* ctx);
    const char* (*artifact_dir)(builder_ctx_t* ctx);
    const char* (*module_dir)(builder_ctx_t* ctx);
} builder_api_t;

typedef void (*builder__build_self_t)(builder_ctx_t* ctx, const builder_api_t* api);
typedef void (*builder__build_module_t)(builder_ctx_t* ctx, const builder_api_t* api, const char* static_libs);

/**
 * Builds the module and installs its artifacts in the path provided in the c_module_t, it must produce API_LIB_NAME in the artifact_dir for dependent modules to use.
 * @param builder All pointers are only valid during the call and must not be retained.
*/
BUILDER_EXTERN void builder__build_self(builder_ctx_t* ctx, const builder_api_t* api);
BUILDER_EXTERN void builder__build_module(builder_ctx_t* ctx, const builder_api_t* api, const char* static_libs);

#endif // MODULES_BUILDER_BUILDER_H
