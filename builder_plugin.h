#ifndef BUILDER_PROJECT_BUILDER_BUILDER_PLUGIN_H
# define BUILDER_PROJECT_BUILDER_BUILDER_PLUGIN_H

# ifdef __cplusplus
#  define BUILDER_EXTERN extern "C"
# else
#  define BUILDER_EXTERN
# endif

# include <stdint.h>
# include <stddef.h>

typedef struct builder_ctx_t builder_ctx_t;
typedef struct builder_api_t builder_api_t;

// package in linking order a complete set of static (pic or non-pic) libraries that other modules can link against
// throw from the implementation if the module doesn't ship as a set of static libraries
BUILDER_EXTERN void builder__export_bundle_static(builder_ctx_t* ctx, const builder_api_t* api);

// package a complete set of shared libraries that other modules can link against
// throw from the implementation if the module doesn't ship as a set of shared libraries
BUILDER_EXTERN void builder__export_bundle_shared(builder_ctx_t* ctx, const builder_api_t* api);

// link any artifacts of the module having access to linking order of the dependencies of the module
BUILDER_EXTERN void builder__link_module(builder_ctx_t* ctx, const builder_api_t* api);

#endif // BUILDER_PROJECT_BUILDER_BUILDER_PLUGIN_H
