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
inline const constexpr char* THIS_WORKSPACE = "ws0";

inline const constexpr char* BUILDER_CPP = "builder.cpp";

inline const constexpr char* WORKSPACES_JSON = "workspaces.json";
inline const constexpr char* MODULE_JSON = "deps.json";

struct module_scc_t;
struct workspace_t;
struct workspace_ecosystem_t;

struct version_t {
    uint64_t value;
};

struct module_t {
    workspace_t* workspace;
    module_scc_t* module_scc;
    filesystem::relative_path_t module_relative_path_to_workspace;
    version_t version;
    std::unordered_set<module_t*> dependencies;
    std::unordered_set<module_t*> builder_dependencies;

    filesystem::path_t source_dir() const;
    filesystem::path_t artifact_base_dir() const;
    filesystem::path_t artifact_dir() const;
    filesystem::path_t artifact_latest_dir() const;

};

struct module_scc_t {
    std::vector<module_t*> modules;
    std::vector<module_scc_t*> dependencies;
};

struct workspace_t {
    workspace_ecosystem_t* workspace_ecosystem;
    filesystem::relative_path_t relative_path;
    uint32_t order_position;
    std::unordered_map<filesystem::relative_path_t, module_t*> module_by_module_relative_path_to_workspace;
};

struct workspace_ecosystem_t {
    filesystem::path_t workspace_root;
    filesystem::path_t artifact_dir;
    std::unordered_map<filesystem::relative_path_t, workspace_t*> workspace_by_relative_path;
    workspace_t* this_workspace;
    module_t* this_module;

    module_t* discover_module(filesystem::relative_path_t workspace_relative_path, filesystem::relative_path_t module_relative_path_to_workspace);
};

version_t derive_version(const std::filesystem::file_time_type& file_time_type);
version_t derive_version(const filesystem::path_t& directory);

} // namespace graph

} // namespace kernel

#endif // KERNEL_GRAPH_H
