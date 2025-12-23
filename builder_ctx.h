#ifndef BUILDER_PROJECT_BUILDER_BUILDER_CTX_H
# define BUILDER_PROJECT_BUILDER_BUILDER_CTX_H

# include "scc.h"

# include "builder_plugin_internal.h"

struct builder_ctx_t {
    builder_ctx_t(module_t* module, const std::vector<scc_t*>& sccs, const std::filesystem::path& artifacts_dir);

    module_t* module;
    const std::vector<scc_t*>& sccs;
    const std::filesystem::path& artifacts_dir;
};

#endif // BUILDER_PROJECT_BUILDER_BUILDER_CTX_H
