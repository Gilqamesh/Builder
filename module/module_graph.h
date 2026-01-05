#ifndef BUILDER_PROJECT_BUILDER_MODULE_MODULE_GRAPH_H
# define BUILDER_PROJECT_BUILDER_MODULE_MODULE_GRAPH_H

# include "../builder.h"

# include <string>
# include <filesystem>
# include <unordered_map>
# include <unordered_set>
# include <vector>
# include <stack>
# include <functional>

# include <cstdint>

class module_t {
public:
    static const constexpr char* BUILDER_CPP = "builder.cpp";
    static const constexpr char* DEPS_JSON = "deps.json";

public:
    module_t(const std::string& name, uint64_t version);

    const std::string& name() const;
    uint64_t version() const;
    void version(uint64_t new_version);

    bool operator==(const module_t& other) const;

private:
    std::string m_name;
    uint64_t m_version;
};

struct module_scc_t {
    std::vector<module_t*> modules;
    std::vector<module_scc_t*> dependencies;
};

class module_graph_t {
public:
    static module_graph_t discover(const std::filesystem::path& modules_dir, const std::string& target_module_name);

    const std::filesystem::path& modules_dir() const;
    const std::filesystem::path& include_dir() const;
    const module_t& builder_module() const;
    const module_t& target_module() const;

    static uint64_t version(const std::filesystem::file_time_type& file_time_type);
    static uint64_t version(const std::filesystem::path& dir);

    const module_scc_t* module_scc(const module_t& module) const;

    void visit_sccs_topo(const module_scc_t* from, const std::function<void(const module_scc_t*)>& f) const;

public: /* debug */
    void svg(const std::filesystem::path& dir, const std::string& file_name_stem);

private:
    module_graph_t(
        std::unordered_map<const module_t*, const module_scc_t*> scc_by_module,
        module_t* builder_module,
        module_t* target_module,
        const std::filesystem::path& modules_dir
    );

    struct discover_result_t {
        module_t* module;
        std::unordered_set<module_t*> dependencies;
    };
    static module_t* discover(const std::filesystem::path& modules_dir, const std::string& module_name, std::unordered_map<std::string, discover_result_t>& discover_results);
    struct module_info_t {
        int index;
        int lowlink;
        bool on_stack;
    };
    static void strong_connect(const std::string& module_name, uint32_t& index, std::unordered_map<std::string, module_info_t>& module_infos_by_module_name, std::stack<std::string>& S, std::unordered_map<std::string, module_scc_t*>& module_sccs, const std::unordered_map<std::string, discover_result_t>& discover_results);
    static uint64_t version_sccs(module_scc_t* scc, std::unordered_map<module_scc_t*, uint64_t>& visited, uint64_t minimum_version);

    void visit_sccs_topo(const module_scc_t* scc, const std::function<void(const module_scc_t*)>& f, std::unordered_set<const module_scc_t*>& visited) const;

private: /* debug */
    void svg_overview(const std::filesystem::path& dir, const std::string& file_name_stem);
    void svg_sccs(const std::filesystem::path& dir, const std::string& file_name_stem);
    void svg_scc(const std::filesystem::path& dir, const module_scc_t* scc, const std::string& file_name_stem);

private:
    std::unordered_map<const module_t*, const module_scc_t*> m_scc_by_module;
    module_t* m_builder_module;
    module_t* m_target_module;
    std::filesystem::path m_modules_dir;
    std::filesystem::path m_include_dir;
};

class builder_t {
public:
    builder_t(const module_graph_t& module_graph, const module_t& module, const std::filesystem::path& artifacts_dir);

    std::vector<std::vector<std::filesystem::path>> export_libraries(library_type_t library_type) const;
    void import_libraries() const;

    std::filesystem::path modules_dir() const;
    std::filesystem::path src_dir() const;
    std::filesystem::path include_dir() const;
    std::filesystem::path artifacts_dir() const;

    std::filesystem::path build_dir(library_type_t library_type) const;
    std::filesystem::path export_dir(library_type_t library_type) const;
    std::filesystem::path import_dir() const;

    std::filesystem::path builder_src_path() const;
    std::filesystem::path builder_build_dir() const;
    std::filesystem::path builder_install_path() const;

private:
    std::vector<std::filesystem::path> export_libraries(const module_t& module, library_type_t library_type) const;
    void import_libraries(const module_t& module) const;

    std::filesystem::path src_dir(const module_t& module) const;
    std::filesystem::path build_dir(const module_t& module, library_type_t library_type) const;
    std::filesystem::path export_dir(const module_t& module, library_type_t library_type) const;
    std::filesystem::path import_dir(const module_t& module) const;

    std::filesystem::path builder_src_path(const module_t& module) const;
    std::filesystem::path builder_build_dir(const module_t& module) const;
    std::filesystem::path builder_install_path(const module_t& module) const;

    std::filesystem::path artifact_dir(const module_t& module) const;
    std::filesystem::path build_dir(const module_t& module) const;
    std::filesystem::path export_dir(const module_t& module) const;

private:
    std::filesystem::path build_builder(const module_t& module) const;

private:
    const module_graph_t& m_module_graph;
    const module_t& m_module;
    std::filesystem::path m_artifacts_dir;
};

#endif // BUILDER_PROJECT_BUILDER_MODULE_MODULE_GRAPH_H
