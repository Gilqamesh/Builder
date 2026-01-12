#include <modules/builder/module/module_graph.h>
#include <modules/builder/compiler/cpp_compiler.h>
#include <modules/builder/json/json.hpp>

#include <format>
#include <fstream>
#include <set>
#include <iostream>

#include <dlfcn.h>

module_t::module_t(const std::string& name, uint64_t version):
    m_name(name),
    m_version(version)
{
}

const std::string& module_t::name() const {
    return m_name;
}

uint64_t module_t::version() const {
    return m_version;
}

void module_t::version(uint64_t new_version) {
    m_version = new_version;
}

bool module_t::operator==(const module_t& other) const {
    return m_name == other.m_name && m_version == other.m_version;
}

builder_t::builder_t(const module_graph_t& module_graph, const module_t& module, const path_t& artifacts_dir):
    m_module_graph(module_graph),
    m_module(module),
    m_artifacts_dir(artifacts_dir)
{
}

std::vector<std::vector<path_t>> builder_t::export_libraries(library_type_t library_type) const {
    std::vector<std::vector<path_t>> library_groups;

    m_module_graph.visit_sccs_topo(m_module_graph.module_scc(m_module), [&](const module_scc_t* scc) {
        std::vector<path_t> library_group;

        for (const auto& module : scc->modules) {
            auto libraries = export_libraries(*module, library_type);
            library_group.insert(library_group.end(), std::make_move_iterator(libraries.begin()), std::make_move_iterator(libraries.end()));
        }

        if (!library_group.empty()) {
            library_groups.emplace_back(std::move(library_group));
        }
    });

    return library_groups;
}

path_t builder_t::modules_dir() const {
    return m_module_graph.modules_dir();
}

path_t builder_t::src_dir() const {
    return src_dir(m_module);
}

path_t builder_t::include_dir() const {
    return m_module_graph.include_dir();
}

path_t builder_t::artifacts_dir() const {
    return m_artifacts_dir;
}

path_t builder_t::build_dir(library_type_t library_type) const {
    return build_dir(m_module, library_type);
}

path_t builder_t::export_dir(library_type_t library_type) const {
    return export_dir(m_module, library_type);
}

path_t builder_t::import_dir() const {
    return import_dir(m_module);
}

path_t builder_t::builder_src_path() const {
    return builder_src_path(m_module);
}

path_t builder_t::builder_build_dir() const {
    return builder_build_dir(m_module);
}

path_t builder_t::builder_install_path() const {
    return builder_install_path(m_module);
}

path_t builder_t::artifact_dir(const module_t& module) const {
    return versioned_path_t::make(artifacts_dir(), module.name(), module.version());
}

path_t builder_t::src_dir(const module_t& module) const {
    return modules_dir() / relative_path_t(module.name());
}

path_t builder_t::build_dir(const module_t& module, library_type_t library_type) const {
    switch (library_type) {
        case LIBRARY_TYPE_STATIC: return build_dir(module) / relative_path_t("static");
        case LIBRARY_TYPE_SHARED: return build_dir(module) / relative_path_t("shared");
        default: throw std::runtime_error(std::format("builder_t::build_dir: unknown library_type {}", (uint32_t)library_type));
    }
}

path_t builder_t::export_dir(const module_t& module, library_type_t library_type) const {
    switch (library_type) {
        case LIBRARY_TYPE_STATIC: return export_dir(module) / relative_path_t("static");
        case LIBRARY_TYPE_SHARED: return export_dir(module) / relative_path_t("shared");
        default: throw std::runtime_error(std::format("builder_t::export_dir: unknown library_type {}", (uint32_t)library_type));
    }
}

path_t builder_t::import_dir(const module_t& module) const {
    return artifact_dir(module) / relative_path_t("import");
}

path_t builder_t::builder_src_path(const module_t& module) const {
    return src_dir(module) / relative_path_t(module_t::BUILDER_CPP);
}

path_t builder_t::builder_build_dir(const module_t& module) const {
    return build_dir(module) / relative_path_t("builder");
}

path_t builder_t::builder_install_path(const module_t& module) const {
    return builder_build_dir(module) / relative_path_t("builder.so");
}

path_t builder_t::build_dir(const module_t& module) const {
    return artifact_dir(module) / relative_path_t("build");
}

path_t builder_t::export_dir(const module_t& module) const {
    return artifact_dir(module) / relative_path_t("export");
}

module_graph_t::module_graph_t(
    std::unordered_map<const module_t*, const module_scc_t*> scc_by_module,
    module_t* builder_module,
    module_t* target_module,
    const path_t& modules_dir
):
    m_scc_by_module(std::move(scc_by_module)),
    m_builder_module(builder_module),
    m_target_module(target_module),
    m_modules_dir(modules_dir),
    m_include_dir(modules_dir.parent())
{
}

const module_scc_t* module_graph_t::module_scc(const module_t& module) const {
    auto it = m_scc_by_module.find(&module);
    if (it == m_scc_by_module.end()) {
        throw std::runtime_error(std::format("module_graph_t::module_scc: module '{}' not found in module graph", module.name()));
    }

    return it->second;
}

module_graph_t module_graph_t::discover(const path_t& modules_dir, const std::string& target_module_name) {
    std::unordered_map<std::string, discover_result_t> discover_results;
    module_t* target_module = discover(modules_dir, target_module_name, discover_results);

    std::unordered_map<std::string, module_info_t> module_infos_by_module_name;
    for (const auto& [module_name, discover_result] : discover_results) {
        module_infos_by_module_name.emplace(module_name, module_info_t { .index = -1, .lowlink = -1, .on_stack = false });
    }

    uint32_t index = 0;
    std::stack<std::string> S;
    std::unordered_map<std::string, module_scc_t*> module_sccs;
    for (const auto& [module_name, discover_result] : discover_results) {
        if (module_infos_by_module_name[module_name].index == -1) {
            strong_connect(module_name, index, module_infos_by_module_name, S, module_sccs, discover_results);
        }
    }

    std::unordered_map<const module_t*, const module_scc_t*> scc_by_module;
    std::unordered_map<module_scc_t*, std::unordered_set<module_scc_t*>> scc_dependencies_by_scc;
    for (const auto& [module_name, discover_result] : discover_results) {
        auto& module_scc = module_sccs[module_name];

        for (const auto& module : module_scc->modules) {
            scc_by_module[module] = module_scc;
        }

        for (const auto& dependency : discover_result.dependencies) {
            auto& dependency_scc = module_sccs[dependency->name()];
            if (module_scc != dependency_scc && scc_dependencies_by_scc[module_scc].insert(dependency_scc).second) {
                module_scc->dependencies.push_back(dependency_scc);
            }
        }
    }

    auto builder_it = discover_results.find("builder");
    module_t* builder_module = nullptr;
    if (builder_it == discover_results.end()) {
        builder_module = new module_t("builder", derive_version(modules_dir / relative_path_t("builder")));
        module_scc_t* builder_module_scc = new module_scc_t;
        builder_module_scc->modules.push_back(builder_module);
        scc_by_module[builder_module] = builder_module_scc;
    } else {
        builder_module = builder_it->second.module;
    }

    std::unordered_map<module_scc_t*, uint64_t> visited;
    version_sccs(module_sccs.at(target_module_name), visited, builder_module->version());

    return module_graph_t(std::move(scc_by_module), builder_module, target_module, modules_dir);
}

void module_graph_t::strong_connect(const std::string& module_name, uint32_t& index, std::unordered_map<std::string, module_info_t>& module_infos_by_module_name, std::stack<std::string>& S, std::unordered_map<std::string, module_scc_t*>& module_sccs, const std::unordered_map<std::string, discover_result_t>& discover_results) {
    module_info_t& module_info = module_infos_by_module_name[module_name];
    module_info.index = index;
    module_info.lowlink = index;
    ++index;
    S.push(module_name);
    module_info.on_stack = true;

    for (const auto& dependency : discover_results.at(module_name).dependencies) {
        module_info_t& dependency_module_info = module_infos_by_module_name[dependency->name()];
        if (dependency_module_info.index == -1) {
            strong_connect(dependency->name(), index, module_infos_by_module_name, S, module_sccs, discover_results);
            module_info.lowlink = std::min(module_info.lowlink, dependency_module_info.lowlink);
        } else if (dependency_module_info.on_stack) {
            module_info.lowlink = std::min(module_info.lowlink, dependency_module_info.index);
        }
    }

    if (module_info.lowlink == module_info.index) {
        module_scc_t* module_scc = new module_scc_t;
        while (1) {
            const auto neighbor_module_name = S.top();
            S.pop();
            module_info_t& neighbor_module_info = module_infos_by_module_name[neighbor_module_name];
            neighbor_module_info.on_stack = false;
            module_scc->modules.emplace_back(discover_results.at(neighbor_module_name).module);
            module_sccs.emplace(neighbor_module_name, module_scc);
            if (module_name == neighbor_module_name) {
                break ;
            }
        }
    }
}

uint64_t module_graph_t::version_sccs(module_scc_t* scc, std::unordered_map<module_scc_t*, uint64_t>& visited, uint64_t minimum_version) {
    if (auto it = visited.find(scc); it != visited.end()) {
        return it->second;
    }

    uint64_t result = minimum_version;

    for (const auto& dependency : scc->dependencies) {
        result = std::max(result, version_sccs(dependency, visited, minimum_version));
    }

    for (const auto& module : scc->modules) {
        result = std::max(result, module->version());
    }

    for (const auto& module : scc->modules) {
        module->version(result);
    }

    visited.emplace(scc, result);

    return result;
}

void module_graph_t::visit_sccs_topo(const module_scc_t* from, const std::function<void(const module_scc_t*)>& f) const {
    std::unordered_set<const module_scc_t*> visited;
    visit_sccs_topo(from, f, visited);
}

void module_graph_t::visit_sccs_topo(const module_scc_t* scc, const std::function<void(const module_scc_t*)>& f, std::unordered_set<const module_scc_t*>& visited) const {
    if (!visited.insert(scc).second) {
        return ;
    }

    for (const auto& dependency : scc->dependencies) {
        visit_sccs_topo(dependency, f, visited);
    }

    f(scc);
}

const path_t& module_graph_t::modules_dir() const {
    return m_modules_dir;
}

const path_t& module_graph_t::include_dir() const {
    return m_include_dir;
}

const module_t& module_graph_t::builder_module() const {
    return *m_builder_module;
}

const module_t& module_graph_t::target_module() const {
    return *m_target_module;
}

uint64_t module_graph_t::derive_version(const std::filesystem::file_time_type& file_time_type) {
    return static_cast<uint64_t>(file_time_type.time_since_epoch().count() - std::numeric_limits<std::filesystem::file_time_type::duration::rep>::min());
}

uint64_t module_graph_t::derive_version(const path_t& dir) {
    // TODO: compare builder bin file hash instead of timestamp for more robust check

    auto latest_module_file = std::filesystem::file_time_type::min();

    for (const auto& path : filesystem_t::find(dir, filesystem_t::include_all, filesystem_t::descend_all)) {
        latest_module_file = std::max(latest_module_file, filesystem_t::last_write_time(path));
    }

    return derive_version(latest_module_file);
}

std::vector<path_t> builder_t::export_libraries(const module_t& module, library_type_t library_type) const {
    const auto& module_export_dir = export_dir(module, library_type);
    if (!filesystem_t::exists(module_export_dir)) {
        if (module == m_module_graph.builder_module()) {
            std::string library_type_str;
            switch (library_type) {
                case LIBRARY_TYPE_STATIC: {
                    library_type_str = "static";
                } break ;
                case LIBRARY_TYPE_SHARED: {
                    library_type_str = "shared";
                } break ;
                default: throw std::runtime_error(std::format("builder_t::export_libraries: unknown library_type {}", (uint32_t)library_type));
            }

            const auto export_command = std::format(
                "make -C \"{}\" BUILD_DIR=\"{}\" EXPORT_DIR=\"{}\" IMPORT_DIR=\"{}\" LIBRARY_TYPE=\"{}\"",
                src_dir(module).string(),
                build_dir(module, library_type).string(),
                export_dir(module, library_type).string(),
                import_dir(module).string(),
                library_type_str
            );
            std::cout << export_command << std::endl;
            const int export_command_result = std::system(export_command.c_str());
            if (export_command_result != 0) {
                throw std::runtime_error(std::format("builder_t::export_libraries: failed to export builder libraries, command exited with code {}", export_command_result));
            }
        } else {
            const auto& builder_plugin = build_builder(module);
            void* builder_plugin_handle = dlopen(builder_plugin.c_str(), RTLD_NOW | RTLD_LOCAL | RTLD_NODELETE);
            if (!builder_plugin_handle) {
                throw std::runtime_error(std::format("builder_t::export_libraries: failed to load builder plugin '{}': {}", builder_plugin.string(), dlerror()));
            }

            try {
                typedef void (*builder__export_libraries_t)(const builder_t* builder, library_type_t library_type);
                builder__export_libraries_t builder__export_libraries = (builder__export_libraries_t) dlsym(builder_plugin_handle, "builder__export_libraries");
                if (!builder__export_libraries) {
                    throw std::runtime_error(std::format("builder_t::export_libraries: failed to load symbol 'builder__export_libraries' from builder plugin '{}': {}", builder_plugin.string(), dlerror()));
                }

                builder_t builder(m_module_graph, module, m_artifacts_dir);
                builder__export_libraries(&builder, library_type);
                dlclose(builder_plugin_handle);

                const auto versioned_modules = filesystem_t::find(
                    m_artifacts_dir / relative_path_t(module.name()),
                    filesystem_t::is_dir,
                    filesystem_t::descend_none
                );

                for (const auto& versioned_module : versioned_modules) {
                    if (versioned_path_t::parse(versioned_module) < module.version()) {
                        filesystem_t::remove_all(versioned_module);
                    }
                }
            } catch (...) {
                dlclose(builder_plugin_handle);
                filesystem_t::remove_all(module_export_dir);
                throw ;
            }
        }
    }

    if (!filesystem_t::exists(module_export_dir)) {
        filesystem_t::create_directories(module_export_dir);
    }

    return filesystem_t::find(module_export_dir, !filesystem_t::is_dir, filesystem_t::descend_all);
}

void builder_t::import_libraries(const module_t& module) const {
    const auto& module_import_dir = import_dir(module);
    if (filesystem_t::exists(module_import_dir) || module == m_module_graph.builder_module()) {
        return ;
    }

    const auto& builder_plugin = build_builder(module);
    void* builder_plugin_handle = dlopen(builder_plugin.c_str(), RTLD_NOW | RTLD_LOCAL | RTLD_NODELETE);
    if (!builder_plugin_handle) {
        throw std::runtime_error(std::format("builder_t::import_libraries failed to load builder plugin '{}': {}", builder_plugin.c_str(), dlerror()));
    }

    try {
        typedef void (*builder__import_libraries_t)(const builder_t* builder);
        builder__import_libraries_t builder__import_libraries = (builder__import_libraries_t)dlsym(builder_plugin_handle, "builder__import_libraries");
        if (!builder__import_libraries) {
            throw std::runtime_error(std::format("builder_t::import_libraries failed to locate symbol 'builder__import_libraries' in module '{}': {}", module.name(), dlerror()));
        }

        builder_t builder(m_module_graph, module, m_artifacts_dir);
        builder__import_libraries(&builder);
        dlclose(builder_plugin_handle);

        if (!filesystem_t::exists(module_import_dir)) {
            filesystem_t::create_directories(module_import_dir);
        }
    } catch (...) {
        dlclose(builder_plugin_handle);
        filesystem_t::remove_all(module_import_dir);
        throw ;
    }
}

path_t builder_t::build_builder(const module_t& module) const {
    const auto builder = builder_install_path(module);
    if (!filesystem_t::exists(builder)) {
        cpp_compiler_t::create_shared_library(
            builder_build_dir(module),
            src_dir(module),
            { include_dir() },
            { builder_src_path(module) },
            {},
            export_libraries(m_module_graph.builder_module(), LIBRARY_TYPE_SHARED),
            builder
        );
    }

    if (!filesystem_t::exists(builder)) {
        throw std::runtime_error(std::format("builder_t::build_builder: expected builder plugin '{}' to exist but it does not", builder.string()));
    }

    return builder;
}

void builder_t::import_libraries() const {
    import_libraries(m_module);
}

void module_graph_t::svg(const path_t& dir, const std::string& file_name_stem) {
    if (!filesystem_t::exists(dir)) {
        filesystem_t::create_directories(dir);
    }

    svg_overview(dir, file_name_stem + "_overview");
    svg_sccs(dir, file_name_stem + "_sccs");
    int scc_id = 0;
    visit_sccs_topo(module_scc(target_module()), [&](const module_scc_t* scc) {
        svg_scc(dir, scc, file_name_stem + "_" + std::to_string(scc_id++));
    });
}

path_t versioned_path_t::make(const path_t& base, std::string_view string_view, uint64_t version) {
    return base / relative_path_t(string_view) / relative_path_t((std::string(string_view) + "@" + std::to_string(version)));
}

uint64_t versioned_path_t::parse(const path_t& path) {
    const auto filename = path.filename();
    const auto at_pos = filename.find_last_of('@');
    if (at_pos == std::string::npos) {
        throw std::runtime_error(std::format("versioned_path_t::parse: path '{}' does not contain version separator '@'", filename));
    }

    const auto version_str = filename.substr(at_pos + 1);
    try {
        return std::stoull(version_str);
    } catch (const std::exception& e) {
        throw std::runtime_error(std::format("versioned_path_t::parse: failed to parse version from path '{}': {}", path.string(), e.what()));
    }
}

module_t* module_graph_t::discover(const path_t& modules_dir, const std::string& module_name, std::unordered_map<std::string, discover_result_t>& discover_results) {
    auto it = discover_results.find(module_name);
    if (it != discover_results.end()) {
        return it->second.module;
    }

    const auto module_dir = modules_dir / relative_path_t(module_name);
    if (!filesystem_t::exists(module_dir)) {
        throw std::runtime_error(std::format("module_graph_t::discover: module directory does not exist '{}'", module_dir.string()));
    }

    auto module = new module_t(module_name, derive_version(module_dir));
    const auto r = discover_results.emplace(module_name, discover_result_t { .module = module, .dependencies = {} });
    it = r.first;

    if (module->name() == "builder") {
        return module;
    }

    const auto builder_cpp = module_dir / relative_path_t(module_t::BUILDER_CPP);
    if (!filesystem_t::exists(builder_cpp)) {
        throw std::runtime_error(std::format("module_graph_t::discover: module '{}' is missing required file '{}'", module_name, builder_cpp.string()));
    }

    const auto deps_json_path = module_dir / relative_path_t(module_t::DEPS_JSON);
    if (!filesystem_t::exists(deps_json_path)) {
        throw std::runtime_error(std::format("module_graph_t::discover: module '{}' is missing required file '{}'", module_name, deps_json_path.string()));
    }

    nlohmann::json deps_json;
    {
        std::ifstream ifs(deps_json_path.string());
        if (!ifs) {
            throw std::runtime_error(std::format("module_graph_t::discover: failed to open file '{}'", deps_json_path.string()));
        }

        try {
            deps_json = nlohmann::json::parse(ifs);
        } catch (const nlohmann::json::parse_error& e) {
            throw std::runtime_error(std::format("module_graph_t::discover: failed to parse json file '{}': {}", deps_json_path.string(), e.what()));
        }
    }

    const auto deps_it = deps_json.find(module_t::DEPS_KEY);
    if (deps_it == deps_json.end()) {
        throw std::runtime_error(std::format("module_graph_t::discover: invalid deps json file '{}': missing '{}' array", deps_json_path.string(), module_t::DEPS_KEY));
    }
    if (!deps_it->is_array()) {
        throw std::runtime_error(std::format("module_graph_t::discover: invalid deps json file '{}': '{}' is not an array", deps_json_path.string(), module_t::DEPS_KEY));
    }
    const auto& module_deps_array = deps_it->get_ref<const nlohmann::json::array_t&>();

    for (const auto& module_deps : module_deps_array) {
        if (!module_deps.is_string()) {
            throw std::runtime_error(std::format("module_graph_t::discover: invalid deps json file '{}': '{}' array must contain only strings", deps_json_path.string(), module_t::DEPS_KEY));
        }
        const std::string module_deps_str = module_deps.get<std::string>();
        if (module_deps_str.empty()) {
            throw std::runtime_error(std::format("module_graph_t::discover: invalid deps json file '{}': '{}' array must not contain empty strings", deps_json_path.string(), module_t::DEPS_KEY));
        }

        it->second.dependencies.insert(discover(modules_dir, module_deps_str, discover_results));
    }

    return module;
}

void module_graph_t::svg_overview(const path_t& dir, const std::string& file_name_stem) {
    if (!filesystem_t::exists(dir)) {
        // TODO: cleanup after fail
        filesystem_t::create_directories(dir);
    }

    const auto dot_file = dir / relative_path_t(file_name_stem + ".dot");
    const auto svg_file = dir / relative_path_t(file_name_stem + ".svg");
    const auto png_file = dir / relative_path_t(file_name_stem + ".png");

    std::ofstream ofs(dot_file.string());
    if (!ofs) {
        throw std::runtime_error("module_graph_t::svg_overview: could not open dot file");
    }

    ofs <<
    R"(digraph OVERVIEW {
        rankdir=LR;
        compound=true;
        newrank=true;
        overlap=false;
        splines=true;
        nodesep=0.35;
        ranksep=0.60;
        fontname="monospace";
        node [shape=box fontname="monospace"];
        edge [fontname="monospace"];
    )";

    int scc_id = 0;
    std::unordered_map<const module_scc_t*, std::string> scc_cluster;
    std::unordered_map<const module_scc_t*, std::string> scc_anchor;
    visit_sccs_topo(module_scc(target_module()), [&](const module_scc_t* scc) {
        const std::string cluster = std::format("cluster_scc{}", scc_id);
        const std::string anchor  = std::format("scc{}_anchor", scc_id);
        scc_cluster[scc] = cluster;
        scc_anchor[scc] = anchor;

        ofs << std::format(
            "    subgraph {} {{\n"
            "        style=rounded;\n"
            "        margin=10;\n"
            "        {} [shape=point width=0 height=0 label=\"\"];\n",
            cluster,
            anchor
        );

        for (const auto& module : scc->modules) {
            ofs << std::format("        {};\n", module->name());
        }

        ofs << "    }\n";

        ++scc_id;
    });

    visit_sccs_topo(module_scc(target_module()), [&](const module_scc_t* scc) {
        for (auto* dependency : scc->dependencies) {
            ofs << std::format(
                "    {} -> {} [ltail={}, lhead={}];\n",
                scc_anchor[scc],
                scc_anchor[dependency],
                scc_cluster[scc],
                scc_cluster[dependency]
            );
        }
    });

    ofs << "}\n";
    ofs.close();

    if (std::system(std::format("dot -Tsvg {} > {}", dot_file.string(), svg_file.string()).c_str()) != 0) {
        throw std::runtime_error("module_graph_t::svg_overview: graphviz render failed");
    }

    if (std::system(std::format("rsvg-convert {} -o {}", svg_file.string(), png_file.string()).c_str()) != 0) {
        throw std::runtime_error("module_graph_t::svg_overview: could not create svg");
    }

    filesystem_t::remove(dot_file);
    // std::filesystem::remove(svg_file);
}

void module_graph_t::svg_sccs(const path_t& dir, const std::string& file_name_stem) {
    if (!filesystem_t::exists(dir)) {
        // TODO: cleanup after fail
        filesystem_t::create_directories(dir);
    }

    const auto dot_file = dir / relative_path_t(file_name_stem + ".dot");
    const auto svg_file = dir / relative_path_t(file_name_stem + ".svg");
    const auto png_file = dir / relative_path_t(file_name_stem + ".png");

    std::ofstream ofs(dot_file.string());
    if (!ofs) {
        throw std::runtime_error("module_graph_t::svg_sccs: could not open dot file");
    }

    ofs <<
    R"(digraph SCCs {
        rankdir=LR;
        node [shape=box fontname="monospace"];
        edge [fontname="monospace"];
    )";

    std::unordered_map<const module_scc_t*, uint32_t> scc_id;
    uint32_t next_id = 0;

    visit_sccs_topo(module_scc(target_module()), [&](const module_scc_t* scc) {
        scc_id[scc] = next_id++;
    });

    visit_sccs_topo(module_scc(target_module()), [&](const module_scc_t* scc) {
        const uint32_t id = scc_id[scc];
        const size_t sz = scc->modules.size();

        double w = 1.2 + 0.15 * std::log2(sz + 1);
        double h = 0.6 + 0.10 * std::log2(sz + 1);

        ofs << std::format(
            "    scc{} [label=\"SCC {}\\n{} nodes\" width={:.2f} height={:.2f} fixedsize=true];\n",
            id, id, sz, w, h
        );
    });

    visit_sccs_topo(module_scc(target_module()), [&](const module_scc_t* scc) {
        const uint32_t from = scc_id[scc];
        for (module_scc_t* dependency : scc->dependencies) {
            const uint32_t to = scc_id[dependency];
            ofs << std::format("    scc{} -> scc{};\n", from, to);
        }
    });

    ofs << "}\n";
    ofs.close();

    if (std::system(std::format("dot -Tsvg {} > {}", dot_file.string(), svg_file.string()).c_str()) != 0) {
        throw std::runtime_error("module_graph_t::svg_sccs: graphviz render failed");
    }

    if (std::system(std::format("rsvg-convert {} -o {}", svg_file.string(), png_file.string()).c_str()) != 0) {
        throw std::runtime_error("module_graph_t::svg_sccs: could not create svg");
    }

    filesystem_t::remove(dot_file);
    // std::filesystem::remove(svg_file);
}

void module_graph_t::svg_scc(const path_t& dir, const module_scc_t* scc, const std::string& file_name_stem) {
    if (!filesystem_t::exists(dir)) {
        // TODO: cleanup after fail
        filesystem_t::create_directories(dir);
    }

    const auto dot_file = dir / relative_path_t(file_name_stem + ".dot");
    const auto svg_file = dir / relative_path_t(file_name_stem + ".svg");
    const auto png_file = dir / relative_path_t(file_name_stem + ".png");

    std::ofstream ofs(dot_file.string());
    if (!ofs)
        throw std::runtime_error("module_graph_t::svg_scc: could not open dot file");

    ofs <<
    R"(digraph SCC_DETAIL {
        layout=neato;
        overlap=scale;
        sep=1.5;
        K=1.2;
        splines=true;
        node [shape=box fontname="monospace"];
        edge [fontname="monospace"];
    )";

    for (const auto& module : scc->modules) {
        ofs << std::format("    {};\n", module->name());
    }

    ofs << "}\n";
    ofs.close();

    if (std::system(std::format("neato -Tsvg {} > {}", dot_file.string(), svg_file.string()).c_str()) != 0) {
        throw std::runtime_error("module_graph_t::svg_scc: graphviz render failed");
    }

    if (std::system(std::format("rsvg-convert {} -o {}", svg_file.string(), png_file.string()).c_str()) != 0) {
        throw std::runtime_error("module_graph_t::svg_scc: could not create svg");
    }

    filesystem_t::remove(dot_file);
    // std::filesystem::remove(svg_file);
}
