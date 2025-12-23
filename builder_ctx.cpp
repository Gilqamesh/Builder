#include "builder_ctx.h"

builder_ctx_t::builder_ctx_t(module_t* module, const std::vector<scc_t*>& sccs, const std::filesystem::path& artifacts_dir):
    module(module),
    sccs(sccs),
    artifacts_dir(artifacts_dir)
{
}
