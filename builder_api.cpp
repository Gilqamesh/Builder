#include "builder_api.h"

#include "compiler.h"

#include <format>

#include <dlfcn.h>

builder_api_t::builder_api_t(uint64_t version):
    version(version)
{
}

std::filesystem::path builder_api_t::modules_dir(builder_ctx_t* ctx) const {
    return ctx->module->modules_dir;
}

std::filesystem::path builder_api_t::source_dir(builder_ctx_t* ctx) const {
    return ctx->module->source_dir;
}

std::filesystem::path builder_api_t::static_bundle_dir(builder_ctx_t* ctx) const {
    return ctx->module->static_bundle_dir();
}

std::filesystem::path builder_api_t::shared_bundle_dir(builder_ctx_t* ctx) const {
    return ctx->module->shared_bundle_dir();
}

std::filesystem::path builder_api_t::linked_artifacts_dir(builder_ctx_t* ctx) const {
    return ctx->module->linked_artifacts_dir();
}

const char* builder_api_t::get_static_link_command_line(builder_ctx_t* ctx) const {
    // visit all scc in reverse topological order and for each module in the scc bundle their static_bundle into a static scc bundle and finally collect the scc bundle
    scc_t* scc = ctx->sccs[ctx->module->scc_id];
    if (!scc->is_static_bundle_link_command_line_initialized) {
        if (!scc->has_static_bundle) {
            collect_static_bundles_from_sccs(scc, ctx);
        }
        collect_static_bundle_link_command_line(scc);
    }

    return scc->static_bundle_link_command_line.c_str();
}

const char* builder_api_t::get_shared_link_command_line(builder_ctx_t* ctx) const {
    // this one is easier, just visit all module dependencies and collect their shared_bundles in whatever order
    if (!ctx->module->is_shared_bundle_link_command_line_initialized) {
        ctx->module->shared_bundle_link_command_line = "";
        std::vector<std::filesystem::path> shared_bundles;
        std::unordered_set<module_t*> visited;
        collect_shared_bundles_from_module_dependencies(ctx, shared_bundles, visited);
        for (const auto& shared_bundle : shared_bundles) {
            if (!ctx->module->shared_bundle_link_command_line.empty()) {
                ctx->module->shared_bundle_link_command_line += " ";
            }
            ctx->module->shared_bundle_link_command_line += std::filesystem::absolute(shared_bundle).string();
        }
    }
    ctx->module->is_shared_bundle_link_command_line_initialized = true;

    return ctx->module->shared_bundle_link_command_line.c_str();
}

void builder_api_t::assert_has_static_libraries(builder_ctx_t* ctx) const {
    if (ctx->module->has_static_bundle) {
        return ;
    }

    const auto static_bundle_dir = ctx->module->artifact_dir / STATIC_BUNDLE_DIR;
    if (!std::filesystem::exists(static_bundle_dir)) {
        if (!std::filesystem::create_directories(static_bundle_dir)) {
            throw module_exception_t(ctx->module, std::format("failed to create static bundle directory '{}'", static_bundle_dir.string()));
        }

        void* builder_plugin = dlopen(ctx->module->builder_plugin.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!builder_plugin) {
            throw module_exception_t(ctx->module, std::format("failed to load builder plugin '{}': {}", ctx->module->builder_plugin.string(), dlerror()));
        }

        try {
            typedef void (*builder__export_bundle_static_t)(builder_ctx_t* ctx, const builder_api_t* api);
            builder__export_bundle_static_t builder__export_bundle_static = (builder__export_bundle_static_t)dlsym(builder_plugin, BUILDER_EXPORT_BUNDLE_STATIC);
            if (!builder__export_bundle_static) {
                throw module_exception_t(ctx->module, std::format("failed to load symbol '{}' from builder plugin '{}': {}", BUILDER_EXPORT_BUNDLE_STATIC, ctx->module->builder_plugin.string(), dlerror()));
            }

            builder__export_bundle_static(ctx, this);
            dlclose(builder_plugin);
        } catch (...) {
            dlclose(builder_plugin);
            throw ;
        }
    } // else { we assume these are the complete set of valid untouched static libraries generated on a previous run }

    for (const auto& entry : std::filesystem::directory_iterator(static_bundle_dir)) {
        if (entry.is_directory() && entry.path().filename() == ARTIFACT_CACHE_DIR) {
            continue ;
        }

        if (!entry.is_regular_file()) {
            throw module_exception_t(ctx->module, std::format("builder plugin '{}' produced unexpected entry '{}' in '{}'", ctx->module->builder_plugin.string(), entry.path().string(), BUILDER_EXPORT_BUNDLE_STATIC));
        }

        const auto& path = entry.path();
        const char* expected_extension = ".lib";
        if (path.extension() != expected_extension) {
            throw module_exception_t(ctx->module, std::format("builder plugin '{}' produced unexpected file '{}' in '{}'", ctx->module->builder_plugin.string(), path.string(), BUILDER_EXPORT_BUNDLE_STATIC));
        }
        ctx->module->static_bundle.push_back(path);
    }

    ctx->module->has_static_bundle = true;
}

void builder_api_t::assert_has_shared_libraries(builder_ctx_t* ctx) const {
    if (ctx->module->has_shared_bundle) {
        return ;
    }

    const auto shared_bundle_dir = ctx->module->artifact_dir / SHARED_BUNDLE_DIR;
    if (!std::filesystem::exists(shared_bundle_dir)) {
        if (!std::filesystem::create_directories(shared_bundle_dir)) {
            throw module_exception_t(ctx->module, std::format("failed to create shared bundle directory '{}'", shared_bundle_dir.string()));
        }

        void* builder_plugin = dlopen(ctx->module->builder_plugin.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!builder_plugin) {
            throw module_exception_t(ctx->module, std::format("failed to load builder plugin '{}': {}", ctx->module->builder_plugin.string(), dlerror()));
        }

        try {
            typedef void (*builder__export_bundle_shared_t)(builder_ctx_t* ctx, const builder_api_t* api);
            builder__export_bundle_shared_t builder__export_bundle_shared = (builder__export_bundle_shared_t)dlsym(builder_plugin, BUILDER_EXPORT_BUNDLE_SHARED);
            if (!builder__export_bundle_shared) {
                throw module_exception_t(ctx->module, std::format("failed to load symbol '{}' from builder plugin '{}': {}", BUILDER_EXPORT_BUNDLE_SHARED, ctx->module->builder_plugin.string(), dlerror()));
            }

            builder__export_bundle_shared(ctx, this);
            dlclose(builder_plugin);
        } catch (...) {
            dlclose(builder_plugin);
            throw ;
        }
    } // else { we assume these are the complete set of valid untouched shared libraries generated on a previous run }

    for (const auto& entry : std::filesystem::directory_iterator(shared_bundle_dir)) {
        if (entry.is_directory() && entry.path().filename() == ARTIFACT_CACHE_DIR) {
            continue ;
        }

        if (!entry.is_regular_file()) {
            throw module_exception_t(ctx->module, std::format("builder plugin '{}' produced unexpected entry '{}' in '{}'", ctx->module->builder_plugin.string(), entry.path().string(), BUILDER_EXPORT_BUNDLE_SHARED));
        }

        const auto& path = entry.path();
        const char* expected_extension = ".so";
        if (path.extension() != expected_extension) {
            throw module_exception_t(ctx->module, std::format("builder plugin '{}' produced unexpected file '{}' in '{}'", ctx->module->builder_plugin.string(), path.string(), BUILDER_EXPORT_BUNDLE_SHARED));
        }
        ctx->module->shared_bundle.push_back(path);
    }

    ctx->module->has_shared_bundle = true;
}

void builder_api_t::assert_has_linked_artifacts(builder_ctx_t* ctx) const {
    if (ctx->module->has_linked_module_artifacts) {
        return ;
    }

    const auto link_module_dir = ctx->module->artifact_dir / LINK_MODULE_DIR;
    if (!std::filesystem::exists(link_module_dir)) {
        if (!std::filesystem::create_directories(link_module_dir)) {
            throw module_exception_t(ctx->module, std::format("failed to create link module directory '{}'", link_module_dir.string()));
        }

        void* builder_plugin = dlopen(ctx->module->builder_plugin.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!builder_plugin) {
            throw module_exception_t(ctx->module, std::format("failed to load builder plugin '{}': {}", ctx->module->builder_plugin.string(), dlerror()));
        }

        try {
            typedef void (*builder__link_module_t)(builder_ctx_t* ctx, const builder_api_t* api);
            builder__link_module_t builder__link_module = (builder__link_module_t)dlsym(builder_plugin, BUILDER_LINK_MODULE);
            if (!builder__link_module) {
                throw module_exception_t(ctx->module, std::format("failed to load symbol '{}' from builder plugin '{}': {}", BUILDER_LINK_MODULE, ctx->module->builder_plugin.string(), dlerror()));
            }

            builder__link_module(ctx, this);
            dlclose(builder_plugin);
        } catch (...) {
            dlclose(builder_plugin);
            throw ;
        }
    } // else { we assume these are the complete set of valid untouched link module generated on a previous run }

    ctx->module->has_linked_module_artifacts = true;
}

void builder_api_t::collect_static_bundles_from_sccs(scc_t* scc, builder_ctx_t* ctx) const {
    if (scc->has_static_bundle) {
        return ;
    }

    // NOTE: fnv1a
    uint64_t hash = 14695981039346656037ull;
    for (const auto& module_by_name : scc->module_by_name) {
        const auto& name = module_by_name.first;
        for (unsigned char c : name) {
            hash ^= c;
            hash *= 1099511628211ull;
        }
    }

    const auto scc_hash_dir = ctx->artifacts_dir / SCC_DIR / std::to_string(hash);
    const auto scc_version_dir = scc_hash_dir / std::to_string(scc->version);
    scc->static_bundle = scc_version_dir / "scc.lib";

    for (const auto& dep : scc->deps) {
        collect_static_bundles_from_sccs(dep, ctx);
    }

    if (std::filesystem::exists(scc->static_bundle)) {
        scc->has_static_bundle = true;
        return ;
    }

    std::vector<std::filesystem::path> archives;
    for (const auto& module_by_name : scc->module_by_name) {
        const auto& module = module_by_name.second;
        builder_ctx_t module_ctx(module, ctx->sccs, ctx->artifacts_dir);
        assert_has_static_libraries(&module_ctx);
        archives.insert(archives.end(), module->static_bundle.begin(), module->static_bundle.end());
    }

    try {
        compiler_t::bundle_static_libraries(archives, scc->static_bundle);

        // remove previous versions to save space
        for (const auto& entry : std::filesystem::directory_iterator(scc_hash_dir)) {
            if (!entry.is_directory()) {
                continue ;
            }

            const auto& path = entry.path();
            const auto& stem = path.stem();
            const uint64_t version = std::stoull(stem.string());
            if (version < scc->version) {
                std::filesystem::remove_all(path);
            }
        }
    } catch (...) {
        std::filesystem::remove_all(scc_version_dir);
        throw ;
    }

    scc->has_static_bundle = true;
}

void builder_api_t::collect_static_bundle_link_command_line(scc_t* scc) const {
    if (scc->is_static_bundle_link_command_line_initialized) {
        return ;
    }

    if (!scc->has_static_bundle) {
        throw std::runtime_error(std::format("internal error: static bundle for scc {} is not generated", scc->id));
    }

    scc->static_bundle_link_command_line = scc->static_bundle.string();
    for (const auto& dep : scc->deps) {
        collect_static_bundle_link_command_line(dep);
        scc->static_bundle_link_command_line += " " + dep->static_bundle_link_command_line;
    }
    scc->is_static_bundle_link_command_line_initialized = true;
}

void builder_api_t::collect_shared_bundles_from_module_dependencies(builder_ctx_t* ctx, std::vector<std::filesystem::path>& shared_bundles, std::unordered_set<module_t*> visited) const {
    if (!visited.insert(ctx->module).second) {
        return ;
    }

    assert_has_shared_libraries(ctx);

    shared_bundles.insert(shared_bundles.end(), ctx->module->shared_bundle.begin(), ctx->module->shared_bundle.end());

    for (const auto& dep : ctx->module->module_dependencies) {
        builder_ctx_t dep_ctx(dep, ctx->sccs, ctx->artifacts_dir);
        collect_shared_bundles_from_module_dependencies(&dep_ctx, shared_bundles, visited);
    }
}
