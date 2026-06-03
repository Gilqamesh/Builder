#ifndef KERNEL_GRAPH_H
# define KERNEL_GRAPH_H

# include "filesystem.h"

# include <cstdint>
# include <utility>
# include <unordered_set>
# include <unordered_map>
# include <vector>

namespace kernel {

namespace graph {

inline const constexpr char* THIS_MODULE = "kernel";
inline const constexpr char* THIS_WORKSPACE = "foundation";

inline const constexpr char* BUILDER_CPP = "builder.cpp";

inline const constexpr char* WORKSPACES_JSON = "workspaces.json";
inline const constexpr char* MODULE_JSON = "deps.json";

class module_t;
class workspace_t;
class workspace_graph_storage_t;
class workspace_graph_t;

struct version_t {
    uint64_t value;
};

using module_group_t = std::vector<module_t*>;

class module_t {
public:
    module_t(workspace_t& workspace, filesystem::relative_path_t name, version_t version);

    workspace_t& workspace() const;
    filesystem::relative_path_t name() const;
    version_t version() const;
    void version(version_t version);
    void add_dependency(module_t& dependency);
    void add_builder_dependency(module_t& dependency);

    std::vector<module_t*> dependencies();
    std::vector<const module_t*> dependencies() const;
    std::vector<module_t*> builder_dependencies();
    std::vector<const module_t*> builder_dependencies() const;
    std::vector<module_group_t> closure_groups() const;
    filesystem::path_t source_dir() const;
    filesystem::path_t artifact_base_dir() const;
    filesystem::path_t artifact_dir() const;
    filesystem::path_t artifact_latest_dir() const;

private:
    workspace_t* m_workspace;
    version_t m_version;
    filesystem::relative_path_t m_name;
    std::unordered_set<module_t*> m_dependencies;
    std::unordered_set<module_t*> m_builder_dependencies;
};

class workspace_t {
public:
    workspace_t(workspace_graph_t& workspace_graph, filesystem::relative_path_t relative_path, uint32_t order_position);

    workspace_graph_t& graph() const;
    const filesystem::relative_path_t& relative_path() const;
    uint32_t order_position() const;
    module_t* find_module(const filesystem::relative_path_t& module_relative_path) const;
    void add_module(module_t* module);
    std::vector<module_t*> modules() const;

private:
    workspace_graph_t* m_workspace_graph;
    filesystem::relative_path_t m_relative_path;
    uint32_t m_order_position;
    std::unordered_map<filesystem::relative_path_t, module_t*> m_module_by_relative_path;
};

class workspace_graph_t {
public:
    workspace_graph_t(filesystem::path_t workspace_root, filesystem::path_t artifact_dir);

    const filesystem::path_t& root() const;
    const filesystem::path_t& artifact_root() const;
    workspace_t& kernel_workspace() const;
    module_t& kernel_module() const;

    module_t* discover_module(filesystem::relative_path_t module_name);
    std::vector<const workspace_t*> workspaces() const;
    std::vector<const module_t*> modules() const;
    std::vector<module_group_t> closure_groups(const module_t& module) const;

private:
    void load_module_index();
    module_t* discover_module_impl(filesystem::relative_path_t module_name);

private:
    std::unordered_map<filesystem::relative_path_t, workspace_t*> m_workspace_by_relative_path;
    std::unordered_map<filesystem::relative_path_t, const workspace_t*> m_workspace_by_module_name;
    filesystem::path_t m_root;
    filesystem::path_t m_artifact_root;
    workspace_t* m_kernel_workspace;
    module_t* m_kernel_module;
    workspace_graph_storage_t* m_storage;
};

version_t derive_version(const std::filesystem::file_time_type& file_time_type);
version_t derive_version(const filesystem::path_t& directory);

} // namespace graph

} // namespace kernel

#endif // KERNEL_GRAPH_H
