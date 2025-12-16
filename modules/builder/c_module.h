#ifndef MODULES_C_MODULE_H
# define MODULES_C_MODULE_H

# ifdef __cplusplus
#  define MODULES_EXTERN extern "C"
# else
#  define MODULES_EXTERN
# endif

typedef enum c_module_state_t {
    NOT_VISITED,
    VISITING,
    VISITED,
    BUILT
} c_module_state_t;

typedef struct c_module_t {
    const char* module_name;
    const char* module_dir;
    const char* root_dir;
    const char* artifact_dir;
    c_module_t** builder_dependencies;
    unsigned int builder_dependencies_count;
    c_module_t** module_dependencies;
    unsigned int module_dependencies_count;
    c_module_state_t state;
    uint64_t version;
} c_module_t;

typedef void (*c_module__build_builder_artifacts_t)(const c_module_t* c_module);
typedef void (*c_module__build_module_artifacts_t)(const c_module_t* c_module);

# ifndef VERSION
#  define VERSION 0
# endif

# define MODULES_DIR           "modules"
# define BUILDER               "builder"
# define BUILDER_CPP           "builder.cpp"
# define BUILDER_SO            "builder.so"
# define ORCHESTRATOR_CPP      "orchestrator.cpp"
# define ORCHESTRATOR_BIN      "orchestrator"
# define BUILDER_PLUGIN_SYMBOL "c_module__build_builder_artifacts"
# define API_LIB_NAME          "api.lib"
# define API_SO_NAME           "api.so"
# define DEPS_JSON             "deps.json"

/**
 * Builds the module and installs its artifacts in the path provided in the c_module_t, it must produce API_LIB_NAME in the artifact_dir for dependent modules to use.
 * @param builder All pointers are only valid during the call and must not be retained.
*/
MODULES_EXTERN void c_module__build_builder_artifacts(const c_module_t* c_module);
MODULES_EXTERN void c_module__build_module_artifacts(const c_module_t* c_module);

#endif // MODULES_C_MODULE_H
