#ifndef BUILDER_CTX_H
# define BUILDER_CTX_H

# include "builder_plugin.h"

# include <string>
# include <filesystem>
# include <unordered_map>
# include <unordered_set>
# include <vector>
# include <stack>

# include <cstdint>

struct module_t {
    module_t(uint32_t id, const std::filesystem::path& module_dir, const std::filesystem::path& artifact_dir, uint64_t version);

    uint32_t id;
    uint32_t scc_id;
    std::filesystem::path module_dir;
    std::filesystem::path artifact_dir;
    uint64_t version;

    struct cached_library_groups_t {
        bool is_cached;
        std::vector<std::vector<std::filesystem::path>> library_groups;
    } cached_library_groups[__BUNDLE_TYPE_SIZE];
};

struct scc_t {
    std::vector<uint32_t> module_ids;
    std::vector<uint32_t> from_sccs;
    uint64_t version;
};

class builder_ctx_t {
public:
    builder_ctx_t(
        const std::filesystem::path& builder_dir,
        const std::filesystem::path& modules_dir,
        const std::string& module_name,
        const std::filesystem::path& artifacts_dir
    );

    const std::filesystem::path& modules_dir() const;
    const std::filesystem::path& module_dir(uint32_t module_id) const;
    const std::filesystem::path& artifacts_dir() const;
    const std::filesystem::path& include_dir() const;

    // TODO: move all this to builder_api?
    std::filesystem::path module_install_dir(uint32_t module_id, bundle_type_t bundle_type) const;
    std::filesystem::path module_cache_dir(uint32_t module_id, bundle_type_t bundle_type) const;
    std::filesystem::path module_build_module_dir(uint32_t module_id) const;
    std::filesystem::path builder_plugin_dir(uint32_t module_id) const;
    std::filesystem::path builder_plugin_cache_dir(uint32_t module_id) const;
    std::filesystem::path builder_plugin_path(uint32_t module_id) const;
    std::filesystem::path builder_src_dir() const;
    std::filesystem::path builder_install_dir(bundle_type_t bundle_type) const;
    std::filesystem::path builder_cache_dir(bundle_type_t bundle_type) const;

    uint64_t version(const std::filesystem::file_time_type& file_time_type) const;
    uint64_t version(const std::filesystem::path& dir) const;
    uint64_t builder_version() const;

    uint32_t discover();

    const std::vector<std::filesystem::path>& build_builder_libraries(bundle_type_t bundle_type);

    void build_module(uint32_t module_id);

    const std::vector<std::vector<std::filesystem::path>>& export_libraries(uint32_t module_id, bundle_type_t bundle_type);

public: /* debug */
    void svg(const std::filesystem::path& dir, const std::string& file_name_stem);

private:
    uint32_t discover(const std::filesystem::path& module_dir, std::unordered_map<std::filesystem::path, uint32_t>& found_modules);

    uint32_t add(const std::filesystem::path& module_dir);

    void connect(uint32_t from, uint32_t to);

    std::filesystem::path artifact_dir(const std::filesystem::path& dir, uint64_t version);

    std::filesystem::path build_builder_plugin(uint32_t module_id);

    struct module_info_t {
        int index;
        int lowlink;
        bool on_stack;
    };

    void update_sccs();

    void strong_connect(uint32_t id, uint32_t& index, std::vector<module_info_t>& module_infos, std::stack<uint32_t>& S);

    void version_sccs(uint32_t scc_id);

    void export_libraries(uint32_t scc_id, bundle_type_t bundle_type, std::vector<bool>& visited, std::vector<std::vector<std::filesystem::path>>& library_groups);

private: /* debug */
    void svg_overview(const std::filesystem::path& dir, const std::string& file_name_stem);
    void svg_sccs(const std::filesystem::path& dir, const std::string& file_name_stem);
    void svg_scc(const std::filesystem::path& dir, uint32_t scc_id, const std::string& file_name_stem);

private:
    std::vector<std::vector<uint32_t>> m_edges;
    std::vector<module_t> m_modules;
    std::vector<scc_t> m_sccs;

    uint64_t m_builder_version;
    std::filesystem::path m_builder_dir;
    std::filesystem::path m_builder_artifact_dir;

    bool m_builder_libraries_cached[__BUNDLE_TYPE_SIZE];
    std::vector<std::filesystem::path> m_builder_libraries[__BUNDLE_TYPE_SIZE];

    std::filesystem::path m_modules_dir;
    std::filesystem::path m_module_dir;
    std::filesystem::path m_artifacts_dir;
    std::filesystem::path m_include_dir;
};

#endif // BUILDER_CTX_H
