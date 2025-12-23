#ifndef BUILDER_PROJECT_BUILDER_MODULE_H
# define BUILDER_PROJECT_BUILDER_MODULE_H

# include <string>
# include <filesystem>
# include <vector>
# include <format>

struct module_t {
    module_t(
        const std::string& name,
        const std::filesystem::path& source_dir,
        const std::filesystem::path& modules_dir,
        const std::filesystem::path& artifact_dir,
        uint64_t version
    );

    std::string name;
    std::filesystem::path source_dir;
    std::filesystem::path modules_dir;
    std::filesystem::path artifact_dir;
    std::vector<module_t*> builder_dependencies;
    std::vector<module_t*> module_dependencies;
    bool is_versioned;
    uint64_t version;
    size_t scc_id;
    bool is_builder_plugin_dependency;
    bool is_module_dependency;

    bool has_static_bundle;
    std::vector<std::filesystem::path> static_bundle;
    bool is_static_bundle_link_command_line_initialized;
    std::string static_bundle_link_command_line;

    bool has_shared_bundle;
    std::vector<std::filesystem::path> shared_bundle;
    bool is_shared_bundle_link_command_line_initialized;
    std::string shared_bundle_link_command_line;

    bool has_linked_module_artifacts;

    std::filesystem::path builder_plugin;

    std::filesystem::path static_bundle_dir();
    std::filesystem::path shared_bundle_dir();
    std::filesystem::path linked_artifacts_dir();
};

class module_exception_t : public std::runtime_error {
public:
    explicit module_exception_t(module_t* module, const std::string& msg);

    module_t* module() const;

private:
    module_t* m_module;
};

#endif // BUILDER_PROJECT_BUILDER_MODULE_H
