#ifndef BUILDER_PROJECT_BUILDER_BUILDER_API_H
# define BUILDER_PROJECT_BUILDER_BUILDER_API_H

# include "builder_ctx.h"

class builder_api_t {
public:
    builder_api_t(uint64_t version);

    const uint64_t version;
    std::filesystem::path modules_dir(builder_ctx_t* ctx) const;
    std::filesystem::path source_dir(builder_ctx_t* ctx) const;
    std::filesystem::path static_bundle_dir(builder_ctx_t* ctx) const;
    std::filesystem::path shared_bundle_dir(builder_ctx_t* ctx) const;
    std::filesystem::path linked_artifacts_dir(builder_ctx_t* ctx) const;
    const char* get_static_link_command_line(builder_ctx_t* ctx) const;
    const char* get_shared_link_command_line(builder_ctx_t* ctx) const;
    // TODO: get linked artifacts

    void assert_has_static_libraries(builder_ctx_t* ctx) const;
    void assert_has_shared_libraries(builder_ctx_t* ctx) const;
    void assert_has_linked_artifacts(builder_ctx_t* ctx) const;

private:
    void collect_static_bundles_from_sccs(scc_t* scc, builder_ctx_t* ctx) const;
    void collect_static_bundle_link_command_line(scc_t* scc) const;
    void collect_shared_bundles_from_module_dependencies(builder_ctx_t* ctx, std::vector<std::filesystem::path>& shared_bundles, std::unordered_set<module_t*> visited) const;
};

#endif // BUILDER_PROJECT_BUILDER_BUILDER_API_H
