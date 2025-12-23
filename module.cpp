#include "module.h"

#include "builder_plugin_internal.h"

module_t::module_t(
    const std::string& name,
    const std::filesystem::path& source_dir,
    const std::filesystem::path& modules_dir,
    const std::filesystem::path& artifact_dir,
    uint64_t version
):
    name(name),
    source_dir(source_dir),
    modules_dir(modules_dir),
    artifact_dir(artifact_dir),
    is_versioned(false),
    version(version),
    scc_id(0),
    is_builder_plugin_dependency(false),
    is_module_dependency(false),
    has_static_bundle(false),
    is_static_bundle_link_command_line_initialized(false),
    has_shared_bundle(false),
    is_shared_bundle_link_command_line_initialized(false),
    has_linked_module_artifacts(false)
{
}

std::filesystem::path module_t::static_bundle_dir() {
    return artifact_dir / STATIC_BUNDLE_DIR;
}

std::filesystem::path module_t::shared_bundle_dir() {
    return artifact_dir / SHARED_BUNDLE_DIR;
}

std::filesystem::path module_t::linked_artifacts_dir() {
    return artifact_dir / LINK_MODULE_DIR;
}

module_exception_t::module_exception_t(module_t* module, const std::string& msg):
    std::runtime_error(msg),
    m_module(module)
{
}

module_t* module_exception_t::module() const {
    return m_module;
}
