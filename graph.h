#ifndef KERNEL_CPP_BUILDER_GRAPH_H
# define KERNEL_CPP_BUILDER_GRAPH_H

# include "external/json.hpp"

# include "filesystem.h"

# include <cstdint>
# include <unordered_set>
# include <unordered_map>
# include <stack>

namespace kernel {

namespace cpp_builder {

namespace builder {

enum class library_type_t : uint8_t;
struct configured_module_t;
struct link_inputs_t;
struct output_t;

} // namespace builder

namespace graph {

enum class link_input_scope_t : uint8_t {
    DEPENDENCIES,
    DEPENDENCIES_AND_SELF
};

inline const constexpr char* THIS_MODULE = "cpp_builder";
inline const constexpr char* THIS_WORKSPACE = "kernel";

inline const constexpr char* BUILDER_CPP = "builder.cpp";

inline const constexpr char* WORKSPACE_JSON = "workspace.json";
struct json_workspace_t {
    uint32_t level;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(json_workspace_t, level)

inline const constexpr char* MODULE_JSON = "deps.json";
struct json_module_dependency_t {
    std::string workspace_relative_path;
    std::string module_relative_path;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(json_module_dependency_t, workspace_relative_path, module_relative_path)

struct json_module_t {
    std::vector<json_module_dependency_t> module_dependencies;
    std::vector<json_module_dependency_t> builder_dependencies;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(json_module_t, module_dependencies, builder_dependencies)

struct module_t;
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
    std::unordered_set<module_t*> producer_dependencies;
    bool validated;
    mutable builder::configured_module_t* static_configured_module = nullptr;
    mutable builder::configured_module_t* shared_configured_module = nullptr;

    filesystem::path_t source_dir() const;
    filesystem::path_t builder_path() const;
    filesystem::path_t artifact_base_dir() const;
    filesystem::path_t artifact_dir() const;
    filesystem::path_t artifact_latest_dir() const;
    filesystem::path_t builder_dir() const;
    filesystem::path_t builder_build_dir() const;
    filesystem::path_t builder_install_dir() const;
    filesystem::path_t builder_install_path() const;
    filesystem::path_t builder_install_latest_path() const;
    filesystem::path_t materialize_builder_plugin() const;
    builder::configured_module_t& configured_module(builder::library_type_t library_type) const;
    builder::link_inputs_t link_inputs(builder::library_type_t library_type, link_input_scope_t scope) const;

    template <class phase_t>
    builder::output_t materialize(builder::library_type_t library_type) const;

    template <class phase_t>
    std::vector<builder::output_t> materialize_all(builder::library_type_t library_type) const;

    void validate();
};

struct module_scc_t {
    std::vector<module_t*> modules;
    std::vector<module_scc_t*> dependencies;
};

struct workspace_t {
    workspace_ecosystem_t* workspace_ecosystem;
    filesystem::relative_path_t workspace_relative_path_to_workspace_ecosystem;
    uint32_t level;
    std::unordered_map<filesystem::relative_path_t, module_t*> module_by_module_relative_path_to_workspace;
};

struct workspace_ecosystem_t {
    filesystem::path_t absolute_path_to_workspace_directory;
    filesystem::path_t artifact_dir;
    std::unordered_map<filesystem::relative_path_t, workspace_t*> workspace_by_workspace_relative_path_to_workspace_ecosystem;
    workspace_t* this_workspace;
    module_t* this_module;

    module_t* discover_module(filesystem::relative_path_t workspace_relative_path_to_workspace_ecosystem, filesystem::relative_path_t module_relative_path_to_workspace);

private:
    struct module_info_t {
        int index;
        int lowlink;
        bool on_stack;
    };

private:
    module_t* discover_module_impl(filesystem::relative_path_t workspace_relative_path_to_workspace_ecosystem, filesystem::relative_path_t module_relative_path_to_workspace);
    void strong_connect(module_t* module, uint32_t& index, std::stack<module_t*>& S, std::unordered_map<module_t*, module_info_t>& module_info_by_module);
    version_t version_sccs(module_scc_t* scc, std::unordered_map<module_scc_t*, version_t>& visited, version_t minimum_version);
};

version_t derive_version(const std::filesystem::file_time_type& file_time_type);
version_t derive_version(const filesystem::path_t& directory);

} // namespace graph

} // namespace cpp_builder

} // namespace kernel

#endif // KERNEL_CPP_BUILDER_GRAPH_H
