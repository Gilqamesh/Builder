#ifndef BUILDER_MODULE_H
# define BUILDER_MODULE_H

# include "filesystem.h"

# include <string>
# include <unordered_map>
# include <unordered_set>
# include <vector>
# include <stack>
# include <functional>

# include <cstdint>

namespace builder {

class module_t {
public:
    static const constexpr char* BUILDER_CPP = "builder.cpp";
    static const constexpr char* DEPS_JSON = "deps.json";
    static const constexpr char* DEPS_KEY = "deps";

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

struct versioned_path_t {
    static filesystem::path_t make(const filesystem::path_t& base, std::string_view string_view, uint64_t version);
    static bool is_versioned(const filesystem::path_t& path);
    static uint64_t parse(const filesystem::path_t& path);
};

class module_graph_t {
public:
    static const constexpr char* DOT_PATH = "/usr/bin/dot";
    static const constexpr char* RSVG_CONVERT_PATH = "/usr/bin/rsvg-convert";
    static const constexpr char* NEATO_PATH = "/usr/bin/neato";

public:
    static module_graph_t discover(const filesystem::path_t& modules_dir, const std::string& target_module_name);

    const filesystem::path_t& modules_dir() const;
    const module_t& builder_module() const;
    const module_t& target_module() const;

    static uint64_t derive_version(const std::filesystem::file_time_type& file_time_type);
    static uint64_t derive_version(const filesystem::path_t& dir);

    const module_scc_t* module_scc(const module_t& module) const;

    void visit_sccs_topo(const module_scc_t* from, const std::function<void(const module_scc_t*)>& f) const;

public: /* debug */
    void svg(const filesystem::path_t& dir, const std::string& file_name_stem);

private:
    module_graph_t(
        std::unordered_map<const module_t*, const module_scc_t*> scc_by_module,
        module_t* builder_module,
        module_t* target_module,
        const filesystem::path_t& modules_dir
    );

    struct discover_result_t {
        module_t* module;
        std::unordered_set<module_t*> dependencies;
    };
    static module_t* discover(const filesystem::path_t& modules_dir, const std::string& module_name, std::unordered_map<std::string, discover_result_t>& discover_results);
    struct module_info_t {
        int index;
        int lowlink;
        bool on_stack;
    };
    static void strong_connect(const std::string& module_name, uint32_t& index, std::unordered_map<std::string, module_info_t>& module_infos_by_module_name, std::stack<std::string>& S, std::unordered_map<std::string, module_scc_t*>& module_sccs, const std::unordered_map<std::string, discover_result_t>& discover_results);
    static uint64_t version_sccs(module_scc_t* scc, std::unordered_map<module_scc_t*, uint64_t>& visited, uint64_t minimum_version);

    void visit_sccs_topo(const module_scc_t* scc, const std::function<void(const module_scc_t*)>& f, std::unordered_set<const module_scc_t*>& visited) const;

private: /* debug */
    void svg_overview(const filesystem::path_t& dir, const std::string& file_name_stem);
    void svg_sccs(const filesystem::path_t& dir, const std::string& file_name_stem);
    void svg_scc(const filesystem::path_t& dir, const module_scc_t* scc, const std::string& file_name_stem);

private:
    std::unordered_map<const module_t*, const module_scc_t*> m_scc_by_module;
    module_t* m_builder_module;
    module_t* m_target_module;
    filesystem::path_t m_modules_dir;
};

} // namespace builder

#endif // BUILDER_MODULE_H
