#ifndef M03GAGBHSP2DRQQ3GKOP8PZFRM_WORKSPACE_GRAPH_H
# define M03GAGBHSP2DRQQ3GKOP8PZFRM_WORKSPACE_GRAPH_H

# include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>

# include <cstdint>
# include <functional>
# include <string>
# include <string_view>
# include <utility>
# include <unordered_set>
# include <unordered_map>
# include <vector>

namespace m03gagbhsp2drqq3gkop8pzfrm_workspace_graph {

inline const constexpr char* BUILDER_CPP = "builder.cpp";
inline const constexpr char* CLI_CPP = "cli.cpp";
inline const constexpr char* WORKSPACES_JSON = "workspaces.json";
inline const constexpr char* MODULE_JSON = "deps.json";

/**
 * Artifact version number.
 */
struct version_t {
    /**
     * Uses value directly.
     */
    explicit version_t(uint64_t value);

    /**
     * Converts a file timestamp to a version number.
     */
    explicit version_t(const std::filesystem::file_time_type& file_time_type);

    /**
     * Uses the latest timestamp under directory.
     */
    explicit version_t(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& directory);

    uint64_t value;
};

/**
 * Workspace and artifact roots for the current process.
 */
struct invocation_context_t {
    m03gagbhsnusi43zogoacgj2ez_filesystem::path_t workspace_root;
    m03gagbhsnusi43zogoacgj2ez_filesystem::path_t artifact_root;
};

/**
 * Non-empty module name.
 */
class module_name_t {
public:
    /**
     * Throws if name is empty.
     */
    explicit module_name_t(std::string_view name);

    const std::string& string() const;
    const char* c_str() const;
    bool operator==(const module_name_t& other) const;

private:
    std::string m_name;
};

/**
 * Hashes module_name_t by its string value.
 */
struct module_name_hash_t {
    std::size_t operator()(const module_name_t& module_name) const noexcept {
        return std::hash<std::string>()(module_name.string());
    }
};

class workspace_t;

/**
 * Discovered module.
 */
class module_t {
public:
    using group_t = std::vector<module_t*>;
    using groups_t = std::vector<group_t>;

    module_t(workspace_t& workspace, module_name_t name, version_t version);

    /**
     * Workspace containing this module.
     */
    workspace_t& workspace() const;

    /**
     * Module name.
     */
    const module_name_t& name() const;

    /**
     * Current artifact version for this module.
     */
    version_t version() const;

    /**
     * Sets the artifact version for this module.
     */
    void version(version_t version);

    /**
     * Adds a module dependency.
     */
    void add_dependency(module_t& dependency);

    /**
     * Adds a builder dependency.
     */
    void add_builder_dependency(module_t& dependency);

    /**
     * Module dependencies sorted by workspace order and name.
     */
    std::vector<module_t*> dependencies();
    std::vector<const module_t*> dependencies() const;

    /**
     * Builder dependencies sorted by workspace order and name.
     */
    std::vector<module_t*> builder_dependencies();
    std::vector<const module_t*> builder_dependencies() const;

    /**
     * Module dependency closure as strongly connected component groups in dependency-to-dependent topological order.
     */
    groups_t closure_groups() const;

    /**
     * Source directory: <workspace_root>/<workspace>/<module>.
     */
    m03gagbhsnusi43zogoacgj2ez_filesystem::path_t source_dir() const;

    /**
     * Artifact base directory: <artifact_root>/<module>.
     */
    m03gagbhsnusi43zogoacgj2ez_filesystem::path_t artifact_base_dir() const;

    /**
     * Versioned artifact directory: <artifact_root>/<module>/<module>@<version>.
     */
    m03gagbhsnusi43zogoacgj2ez_filesystem::path_t artifact_dir() const;

    /**
     * Latest artifact directory: <artifact_root>/<module>/latest.
     */
    m03gagbhsnusi43zogoacgj2ez_filesystem::path_t artifact_latest_dir() const;

private:
    workspace_t* m_workspace;
    version_t m_version;
    module_name_t m_name;
    std::unordered_set<module_t*> m_dependencies;
    std::unordered_set<module_t*> m_builder_dependencies;
};

class workspace_graph_t;

/**
 * Workspace entry loaded from workspaces.json.
 */
class workspace_t {
public:
    workspace_t(workspace_graph_t& workspace_graph, m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t relative_path, uint32_t order_position);

    /**
     * Graph containing this workspace.
     */
    workspace_graph_t& graph() const;

    /**
     * Workspace path relative to the graph root.
     */
    const m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t& relative_path() const;

    /**
     * Zero-based position in workspaces.json.
     */
    uint32_t order_position() const;

    /**
     * Returns a discovered module by name, or nullptr.
     */
    module_t* find_module(const module_name_t& module_name) const;

    /**
     * Adds a discovered module to this workspace.
     */
    void add_module(module_t* module);

    /**
     * Discovered modules sorted by workspace order and name.
     */
    std::vector<module_t*> modules() const;

private:
    workspace_graph_t* m_workspace_graph;
    m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t m_relative_path;
    uint32_t m_order_position;
    std::unordered_map<module_name_t, module_t*, module_name_hash_t> m_module_by_name;
};

class workspace_graph_storage_t;

/**
 * Module graph for one workspace root and artifact root.
 */
class workspace_graph_t {
public:
    workspace_graph_t(m03gagbhsnusi43zogoacgj2ez_filesystem::path_t workspace_root, m03gagbhsnusi43zogoacgj2ez_filesystem::path_t artifact_dir);

    /**
     * Workspace root directory.
     */
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& root() const;

    /**
     * Artifact root directory.
     */
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& artifact_root() const;

    /**
     * Module used as the active Builder bootstrap seed.
     */
    module_t& bootstrap_seed_module() const;

    /**
     * True for modules participating in the active Builder bootstrap group.
     */
    bool is_active_builder_bootstrap_module(const module_t& module) const;

    /**
     * Discovers module_name, its reachable dependencies, and validates them.
     */
    module_t* discover_module(module_name_t module_name);

    /**
     * Workspaces sorted by workspaces.json order.
     */
    std::vector<const workspace_t*> workspaces() const;

    /**
     * Discovered modules sorted by workspace order and name.
     */
    std::vector<const module_t*> modules() const;

    /**
     * Module dependency closure as strongly connected component groups in dependency-to-dependent topological order.
     */
    module_t::groups_t closure_groups(const module_t& module) const;

private:
    void load_module_index();
    module_t* discover_module_impl(module_name_t module_name);

private:
    std::unordered_map<m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t, workspace_t*> m_workspace_by_relative_path;
    std::unordered_map<module_name_t, const workspace_t*, module_name_hash_t> m_workspace_by_module_name;
    m03gagbhsnusi43zogoacgj2ez_filesystem::path_t m_root;
    m03gagbhsnusi43zogoacgj2ez_filesystem::path_t m_artifact_root;
    workspace_t* m_bootstrap_seed_workspace;
    module_t* m_bootstrap_seed_module;
    workspace_graph_storage_t* m_storage;
};

/**
 * Reads BUILDER_WORKSPACE_ROOT and BUILDER_ARTIFACT_ROOT, applies defaults, and exports the selected values.
 */
invocation_context_t invocation_context();

} // namespace m03gagbhsp2drqq3gkop8pzfrm_workspace_graph

namespace std {

template <>
struct formatter<m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t> : formatter<std::string> {
    auto format(const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t& module_name, auto& ctx) const {
        return formatter<std::string>::format(module_name.string(), ctx);
    }
};

} // namespace std

#endif // M03GAGBHSP2DRQQ3GKOP8PZFRM_WORKSPACE_GRAPH_H
