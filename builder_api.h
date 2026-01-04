#ifndef BUILDER_API_H
# define BUILDER_API_H

# include "builder_ctx.h"

class builder_api_t {
public:
    builder_api_t(uint32_t module_id);

    std::filesystem::path source_dir(builder_ctx_t* ctx) const;
    std::filesystem::path include_dir(builder_ctx_t* ctx) const;
    std::filesystem::path install_dir(builder_ctx_t* ctx, bundle_type_t bundle_type) const;
    std::filesystem::path cache_dir(builder_ctx_t* ctx, bundle_type_t bundle_type) const; // TODO: this is tmp dir really, caching is unused
    std::filesystem::path build_module_dir(builder_ctx_t* ctx) const;
    std::filesystem::path builder_plugin_dir(builder_ctx_t* ctx) const;
    std::filesystem::path builder_plugin_cache_dir(builder_ctx_t* ctx) const;
    std::filesystem::path builder_plugin_path(builder_ctx_t* ctx) const;
    const std::vector<std::vector<std::filesystem::path>>& export_libraries(builder_ctx_t* ctx, bundle_type_t bundle_type) const;

private:
    const uint32_t m_module_id;
};

#endif // BUILDER_API_H
