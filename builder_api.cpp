#include "builder_api.h"

#include "builder_ctx.h"

builder_api_t::builder_api_t(uint32_t module_id):
    m_module_id(module_id)
{
}

std::filesystem::path builder_api_t::source_dir(builder_ctx_t* ctx) const {
    return ctx->module_dir(m_module_id);
}

std::filesystem::path builder_api_t::include_dir(builder_ctx_t* ctx) const {
    return ctx->include_dir();
}

std::filesystem::path builder_api_t::install_dir(builder_ctx_t* ctx, bundle_type_t bundle_type) const {
    return ctx->module_install_dir(m_module_id, bundle_type);
}

std::filesystem::path builder_api_t::cache_dir(builder_ctx_t* ctx, bundle_type_t bundle_type) const {
    return ctx->module_cache_dir(m_module_id, bundle_type);
}

std::filesystem::path builder_api_t::build_module_dir(builder_ctx_t* ctx) const {
    return ctx->module_build_module_dir(m_module_id);
}

std::filesystem::path builder_api_t::builder_plugin_dir(builder_ctx_t* ctx) const {
    return ctx->builder_plugin_dir(m_module_id);
}

std::filesystem::path builder_api_t::builder_plugin_cache_dir(builder_ctx_t* ctx) const {
    return ctx->builder_plugin_cache_dir(m_module_id);
}

std::filesystem::path builder_api_t::builder_plugin_path(builder_ctx_t* ctx) const {
    return ctx->builder_plugin_path(m_module_id);
}

const std::vector<std::vector<std::filesystem::path>>& builder_api_t::export_libraries(builder_ctx_t* ctx, bundle_type_t bundle_type) const {
    return ctx->export_libraries(m_module_id, bundle_type);
}
