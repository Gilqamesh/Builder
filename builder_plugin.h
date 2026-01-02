#ifndef BUILDER_PLUGIN_H
# define BUILDER_PLUGIN_H

# ifdef __cplusplus
#  define BUILDER_EXTERN extern "C"
# else
#  define BUILDER_EXTERN
# endif

typedef struct builder_ctx_t builder_ctx_t;
typedef struct builder_api_t builder_api_t;

typedef enum bundle_type_t {
    BUNDLE_TYPE_STATIC,
    BUNDLE_TYPE_SHARED,

    __BUNDLE_TYPE_SIZE
} bundle_type_t;

BUILDER_EXTERN void builder__export_libraries(builder_ctx_t* ctx, const builder_api_t* api, bundle_type_t bundle_type);
BUILDER_EXTERN void builder__build_module(builder_ctx_t* ctx, const builder_api_t* api);

#endif // BUILDER_PLUGIN_H
