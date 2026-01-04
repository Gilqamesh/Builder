#include <builder/builder_ctx.h>
#include <builder/builder_api.h>
#include <builder/compiler/cpp_compiler.h>
#include <builder/find/find.h>
#include <builder/internal.h>
#include <builder/json/json.hpp>

#include <format>
#include <fstream>
#include <set>
#include <iostream>

#include <dlfcn.h>

builder_ctx_t::builder_ctx_t(
    const std::filesystem::path& builder_dir,
    const std::filesystem::path& modules_dir,
    const std::string& module_name,
    const std::filesystem::path& artifacts_dir
):
    m_builder_version(0),
    m_builder_dir(builder_dir),
    m_modules_dir(modules_dir),
    m_module_dir(modules_dir / module_name),
    m_artifacts_dir(artifacts_dir)
{
    m_include_dir = m_modules_dir.parent_path();
    if (m_include_dir.empty()) {
        m_include_dir = ".";
    }

    m_builder_version = version(builder_dir);
    m_builder_artifact_dir = artifact_dir(builder_dir, m_builder_version);

    for (int i = 0; i < sizeof(m_builder_libraries_cached) / sizeof(m_builder_libraries_cached[0]); ++i) {
        m_builder_libraries_cached[i] = false;
    }
}

const std::filesystem::path& builder_ctx_t::modules_dir() const {
    return m_modules_dir;
}

const std::filesystem::path& builder_ctx_t::module_dir(uint32_t module_id) const {
    if ((uint32_t)m_modules.size() <= module_id) {
        throw std::runtime_error(std::format("module_dir: invalid module_id {}", module_id));
    }

    return m_modules[module_id].module_dir;
}

const std::filesystem::path& builder_ctx_t::artifacts_dir() const {
    return m_artifacts_dir;
}

const std::filesystem::path& builder_ctx_t::include_dir() const {
    return m_include_dir;
}

std::filesystem::path builder_ctx_t::build_builder_plugin(uint32_t module_id) {
    const auto builder_plugin = builder_plugin_path(module_id);
    if (!std::filesystem::exists(builder_plugin)) {
        cpp_compiler_t::create_loadable(
            builder_plugin_cache_dir(module_id),
            module_dir(module_id),
            { include_dir() },
            { module_dir(module_id) / BUILDER_PLUGIN_CPP },
            {},
            { build_builder_libraries(BUNDLE_TYPE_SHARED) },
            BUNDLE_TYPE_SHARED,
            builder_plugin
        );
    }

    if (!std::filesystem::exists(builder_plugin)) {
        throw std::runtime_error(std::format("build_builder_plugin: expected builder plugin '{}' to exist but it does not", builder_plugin.string()));
    }

    return builder_plugin;
}

std::filesystem::path builder_ctx_t::module_install_dir(uint32_t module_id, bundle_type_t bundle_type) const {
    switch (bundle_type) {
        case BUNDLE_TYPE_STATIC: return m_modules[module_id].artifact_dir / "static" / "install";
        case BUNDLE_TYPE_SHARED: return m_modules[module_id].artifact_dir / "shared" / "install";
        default: throw std::runtime_error(std::format("module_install_dir: unknown bundle_type {}", (uint32_t)bundle_type));
    }
}

std::filesystem::path builder_ctx_t::module_cache_dir(uint32_t module_id, bundle_type_t bundle_type) const {
    switch (bundle_type) {
        case BUNDLE_TYPE_STATIC: return m_modules[module_id].artifact_dir / "static" / "cache";
        case BUNDLE_TYPE_SHARED: return m_modules[module_id].artifact_dir / "shared" / "cache";
        default: throw std::runtime_error(std::format("module_cache_dir: unknown bundle_type {}", (uint32_t)bundle_type));
    }
}

std::filesystem::path builder_ctx_t::module_build_module_dir(uint32_t module_id) const {
    return m_modules[module_id].artifact_dir / "module";
}

std::filesystem::path builder_ctx_t::builder_plugin_dir(uint32_t module_id) const {
    return m_modules[module_id].artifact_dir / "builder_plugin";
}

std::filesystem::path builder_ctx_t::builder_plugin_cache_dir(uint32_t module_id) const {
    return builder_plugin_dir(module_id) / "cache";
}

std::filesystem::path builder_ctx_t::builder_plugin_path(uint32_t module_id) const {
    return builder_plugin_dir(module_id) / BUILDER_PLUGIN_SO;
}

std::filesystem::path builder_ctx_t::builder_src_dir() const {
    return m_builder_dir;
}

std::filesystem::path builder_ctx_t::builder_install_dir(bundle_type_t bundle_type) const {
    switch (bundle_type) {
        case BUNDLE_TYPE_STATIC: return m_builder_artifact_dir / "static" / "install";
        case BUNDLE_TYPE_SHARED: return m_builder_artifact_dir / "shared" / "install";
        default: throw std::runtime_error(std::format("builder_install_dir: unknown bundle_type {}", (uint32_t)bundle_type));
    }
}

std::filesystem::path builder_ctx_t::builder_cache_dir(bundle_type_t bundle_type) const {
    switch (bundle_type) {
        case BUNDLE_TYPE_STATIC: return m_builder_artifact_dir / "static" / "cache";
        case BUNDLE_TYPE_SHARED: return m_builder_artifact_dir / "shared" / "cache";
        default: throw std::runtime_error(std::format("builder_cache_dir: unknown bundle_type {}", (uint32_t)bundle_type));
    }
}

uint64_t builder_ctx_t::version(const std::filesystem::file_time_type& file_time_type) const {
    return static_cast<uint64_t>(file_time_type.time_since_epoch().count() - std::numeric_limits<std::filesystem::file_time_type::duration::rep>::min());
}

uint64_t builder_ctx_t::version(const std::filesystem::path& dir) const {
    // TODO: compare builder bin file hash instead of timestamp for more robust check

    auto latest_module_file = std::filesystem::file_time_type::min();

    std::error_code ec;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(dir, ec)) {
        latest_module_file = std::max(latest_module_file, std::filesystem::last_write_time(entry.path()));
    }

    if (ec) {
        throw std::runtime_error(std::format("version: failed to recursively iterate module directory '{}': {}", dir.string(), ec.message()));
    }

    return version(latest_module_file);
}

uint64_t builder_ctx_t::builder_version() const {
    return m_builder_version;
}

uint32_t builder_ctx_t::discover() {
    std::unordered_map<std::filesystem::path, uint32_t> found_modules;
    uint32_t result = discover(m_module_dir, found_modules);

    update_sccs();

    return result;
}

void builder_ctx_t::build_builder_core(bundle_type_t bundle_type) {
   cpp_compiler_t::create_library(
        builder_cache_dir(bundle_type),
        builder_src_dir(),
        { builder_src_dir() },
        find_t::find(
            m_builder_dir,
            [](const std::filesystem::directory_entry& entry) {
                const auto& path = entry.path();
                if (path.extension() != ".cpp") {
                    return false;
                }

                const auto filename = path.filename();
                return filename != "cli.cpp" && filename != "test.cpp";
            },
            false
        ),
        {},
        builder_install_dir(bundle_type) / "core",
        bundle_type
    );
}

void builder_ctx_t::build_builder_compiler(bundle_type_t bundle_type) {
    cpp_compiler_t::create_library(
        builder_cache_dir(bundle_type),
        builder_src_dir(),
        { builder_src_dir() },
        find_t::find(
            m_builder_dir / "compiler",
            [](const std::filesystem::directory_entry& entry) {
                const auto& path = entry.path();
                if (path.extension() != ".cpp") {
                    return false;
                }

                const auto filename = path.filename();
                return filename != "cli.cpp" && filename != "test.cpp";
            },
            false
        ),
        {},
        builder_install_dir(bundle_type) / "zip",
        bundle_type
    );
}

void builder_ctx_t::build_builder_curl(bundle_type_t bundle_type) {
    if (bundle_type != BUNDLE_TYPE_SHARED) {
        throw std::runtime_error("build_builder_curl: only shared bundle type is supported");
    }

    const auto system_curl = std::filesystem::path("/usr/lib64/libcurl.so");
    if (!std::filesystem::exists(system_curl)) {
        throw std::runtime_error(std::format("system curl library '{}' does not exist, TODO: extend logic to install it", system_curl.string()));
    }

    const auto install_dir = builder_install_dir(bundle_type);

    cpp_compiler_t::reference_shared_library(system_curl, install_dir / "curl-ref.so");

    cpp_compiler_t::create_shared_library(
        builder_cache_dir(bundle_type),
        builder_src_dir(),
        { builder_src_dir() },
        find_t::find(
            m_builder_dir / "curl",
            [](const std::filesystem::directory_entry& entry) {
                const auto& path = entry.path();
                if (path.extension() != ".cpp") {
                    return false;
                }

                const auto filename = path.filename();
                return filename != "cli.cpp" && filename != "test.cpp";
            },
            false
        ),
        {},
        install_dir / "curl.so"
    );
}

void builder_ctx_t::build_builder_find(bundle_type_t bundle_type) {
    cpp_compiler_t::create_library(
        builder_cache_dir(bundle_type),
        builder_src_dir(),
        { builder_src_dir() },
        find_t::find(
            m_builder_dir / "find",
            [](const std::filesystem::directory_entry& entry) {
                const auto& path = entry.path();
                if (path.extension() != ".cpp") {
                    return false;
                }

                const auto filename = path.filename();
                return filename != "cli.cpp" && filename != "test.cpp";
            },
            false
        ),
        {},
        builder_install_dir(bundle_type) / "find",
        bundle_type
    );
}

void builder_ctx_t::build_builder_json(bundle_type_t bundle_type) {
    cpp_compiler_t::create_library(
        builder_cache_dir(bundle_type),
        builder_src_dir(),
        { builder_src_dir() },
        find_t::find(
            m_builder_dir / "json",
            [](const std::filesystem::directory_entry& entry) {
                const auto& path = entry.path();
                if (path.extension() != ".cpp") {
                    return false;
                }

                const auto filename = path.filename();
                return filename != "cli.cpp";
            },
            false
        ),
        {},
        builder_install_dir(bundle_type) / "json",
        bundle_type
    );
}

void builder_ctx_t::build_builder_zip(bundle_type_t bundle_type) {
    cpp_compiler_t::create_library(
        builder_cache_dir(bundle_type),
        builder_src_dir(),
        { builder_src_dir() },
        find_t::find(
            m_builder_dir / "zip",
            [](const std::filesystem::directory_entry& entry) {
                const auto& path = entry.path();
                if (path.extension() != ".cpp") {
                    return false;
                }

                const auto filename = path.filename();
                return filename != "cli.cpp" && filename != "test.cpp";
            },
            true
        ),
        {},
        builder_install_dir(bundle_type) / "zip",
        bundle_type
    );
}

const std::vector<std::filesystem::path>& builder_ctx_t::build_builder_libraries(bundle_type_t bundle_type) {
    if (m_builder_libraries_cached[bundle_type]) {
        return m_builder_libraries[bundle_type];
    }

    if (!std::filesystem::exists(builder_install_dir(bundle_type))) {
        m_builder_libraries[bundle_type].clear();

        std::string library_type;
        switch (bundle_type) {
            case BUNDLE_TYPE_STATIC: {
                library_type = "static";
            } break ;
            case BUNDLE_TYPE_SHARED: {
                library_type = "shared";
            } break ;
            default: throw std::runtime_error(std::format("build_builder_libraries: unknown bundle_type {}", (uint32_t)bundle_type));
        }

        const auto build_command = std::format(
            "make -C \"{}\" LIBRARY_TYPE=\"{}\" INSTALL_ROOT=\"{}\"",
            std::filesystem::absolute(builder_src_dir()).string(),
            library_type,
            std::filesystem::absolute(m_builder_artifact_dir).string()
        );
        std::cout << build_command << std::endl;
        const int build_command_result = std::system(build_command.c_str());
        if (build_command_result != 0) {
            throw std::runtime_error(std::format("build_builder_libraries: failed to build builder libraries, command exited with code {}", build_command_result));
        }
    }

    m_builder_libraries[bundle_type] = find_t::find(
        builder_install_dir(bundle_type),
        [](const std::filesystem::directory_entry& entry) { return true; },
        true
    );

    m_builder_libraries_cached[bundle_type] = true;

    return m_builder_libraries[bundle_type];
}

void builder_ctx_t::build_module(uint32_t module_id) {
    const auto build_module_dir = module_build_module_dir(module_id);
    if (std::filesystem::exists(build_module_dir)) {
        return ;
    }

    const auto builder_plugin = build_builder_plugin(module_id);
    if (!std::filesystem::exists(builder_plugin)) {
        throw std::runtime_error(std::format("build_module: builder plugin '{}' does not exist", builder_plugin.string()));
    }

    void* builder_plugin_handle = dlopen(builder_plugin.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!builder_plugin_handle) {
        throw std::runtime_error(std::format("build_module: failed to load builder plugin '{}': {}", builder_plugin.c_str(), dlerror()));
    }

    try {
        typedef void (*builder__build_module_t)(builder_ctx_t* ctx, const builder_api_t* api);
        builder__build_module_t builder__build_module = (builder__build_module_t)dlsym(builder_plugin_handle, "builder__build_module");
        if (!builder__build_module) {
            dlclose(builder_plugin_handle);
            throw std::runtime_error(std::format("build_module: failed to locate symbol 'builder__build_module' in builder plugin '{}': {}", builder_plugin.c_str(), dlerror()));
        }

        builder_api_t api(module_id);
        builder__build_module(this, &api);
        dlclose(builder_plugin_handle);
    } catch (...) {
        dlclose(builder_plugin_handle);
        std::filesystem::remove_all(build_module_dir);
        throw ;
    }
}

const std::vector<std::vector<std::filesystem::path>>& builder_ctx_t::export_libraries(uint32_t module_id, bundle_type_t bundle_type) {
    auto& module = m_modules[module_id];
    auto& cached_library_groups = module.cached_library_groups[bundle_type];
    if (!cached_library_groups.is_cached) {
        cached_library_groups.library_groups.clear();
        std::vector<bool> visited(m_sccs.size(), false);
        export_libraries(module.scc_id, bundle_type, visited, cached_library_groups.library_groups);
        cached_library_groups.is_cached = true;
    }

    return cached_library_groups.library_groups;
}

void builder_ctx_t::svg(const std::filesystem::path& dir, const std::string& file_name_stem) {
    if (!std::filesystem::exists(dir)) {
        if (!std::filesystem::create_directories(dir)) {
            throw std::runtime_error(std::format("svg: failed to create directory '{}'", dir.string()));
        }
    }

    svg_overview(dir, file_name_stem + "_overview");
    svg_sccs(dir, file_name_stem + "_sccs");
    for (uint32_t scc_id = 0; scc_id < m_sccs.size(); ++scc_id) {
        svg_scc(dir, scc_id, file_name_stem + "_" + std::to_string(scc_id));
    }
}

module_t::module_t(uint32_t id, const std::filesystem::path& module_dir, const std::filesystem::path& artifact_dir, uint64_t version):
    id(id),
    scc_id(0),
    module_dir(module_dir),
    artifact_dir(artifact_dir),
    version(version)
{
    for (int i = 0; i < sizeof(cached_library_groups) / sizeof(cached_library_groups[0]); ++i) {
        cached_library_groups[i].is_cached = false;
    }
}

uint32_t builder_ctx_t::discover(const std::filesystem::path& module_dir, std::unordered_map<std::filesystem::path, uint32_t>& found_modules) {
    auto it = found_modules.find(module_dir);
    if (it != found_modules.end()) {
        return it->second;
    }

    if (!std::filesystem::exists(module_dir)) {
        throw std::runtime_error(std::format("discover: module directory does not exist '{}'", module_dir.string()));
    }

    const auto builder_plugin_cpp = module_dir / BUILDER_PLUGIN_CPP;
    if (!std::filesystem::exists(builder_plugin_cpp)) {
        throw std::runtime_error(std::format("discover: module '{}' is missing required file '{}'", module_dir.stem().string(), builder_plugin_cpp.string()));
    }

    const uint32_t module_id = add(module_dir);
    const auto r = found_modules.emplace(module_dir, module_id);
    if (!r.second) {
        throw std::runtime_error(std::format("discover: internal error"));
    }
    it = r.first;

    const auto deps_json_path = module_dir / DEPS_JSON;
    if (!std::filesystem::exists(deps_json_path)) {
        throw std::runtime_error(std::format("discover: file does not exist '{}'", deps_json_path.string()));
    }

    nlohmann::json deps_json;
    {
        std::ifstream ifs(deps_json_path);
        if (!ifs) {
            throw std::runtime_error(std::format("discover: failed to open file '{}'", deps_json_path.string()));
        }

        try {
            deps_json = nlohmann::json::parse(ifs);
        } catch (const nlohmann::json::parse_error& e) {
            throw std::runtime_error(std::format("discover: failed to parse json file '{}': {}", deps_json_path.string(), e.what()));
        }
    }

    const auto deps_it = deps_json.find(DEPS_KEY);
    if (deps_it == deps_json.end()) {
        throw std::runtime_error(std::format("discover: invalid deps json file '{}': missing '{}' array", deps_json_path.string(), DEPS_KEY));
    }
    if (!deps_it->is_array()) {
        throw std::runtime_error(std::format("discover: invalid deps json file '{}': '{}' is not an array", deps_json_path.string(), DEPS_KEY));
    }
    const auto& module_deps = deps_it->get_ref<const nlohmann::json::array_t&>();
    if (module_deps.size() != std::unordered_set<std::string>(module_deps.begin(), module_deps.end()).size()) {
        throw std::runtime_error(std::format("discover: invalid deps json file '{}': '{}' array has duplicates", deps_json_path.string(), DEPS_KEY));
    }
    for (const auto& module_deps : module_deps) {
        if (!module_deps.is_string()) {
            throw std::runtime_error(std::format("discover: invalid deps json file '{}': '{}' array must contain only strings", deps_json_path.string(), DEPS_KEY));
        }
        const std::string module_deps_str = module_deps.get<std::string>();
        if (module_deps_str.empty()) {
            throw std::runtime_error(std::format("discover: invalid deps json file '{}': '{}' array must not contain empty strings", deps_json_path.string(), DEPS_KEY));
        }

        const uint32_t from_module_id = discover(m_modules_dir / module_deps_str, found_modules);
        connect(from_module_id, module_id);
    }

    return module_id;
}

uint32_t builder_ctx_t::add(const std::filesystem::path& module_dir) {
    const auto module_version = version(module_dir);
    const auto module_artifact_dir = artifact_dir(module_dir, module_version);
    const auto module_id = m_modules.size();

    module_t module(module_id, module_dir, module_artifact_dir, module_version);
    m_modules.emplace_back(std::move(module));
    m_edges.push_back({});

    return module_id;
}

void builder_ctx_t::connect(uint32_t from, uint32_t to) {
    auto it = std::find(m_edges[to].begin(), m_edges[to].end(), from);
    if (it != m_edges[to].end()) {
        return ;
    }
    // TODO: assert that no duplicate edges
    m_edges[to].push_back(from);
}

std::filesystem::path builder_ctx_t::artifact_dir(const std::filesystem::path& dir, uint64_t version) {
    const std::string dir_name = dir.stem();
    return m_artifacts_dir / dir_name / (dir_name + "@" + std::to_string(version));
}

void builder_ctx_t::update_sccs() {
    m_sccs.clear();

    std::vector<module_info_t> module_infos(m_modules.size(), module_info_t{ .index = -1, .lowlink = -1, .on_stack = false });
    uint32_t index = 0;
    std::stack<uint32_t> S;
    for (uint32_t module_id = 0; module_id < module_infos.size(); ++module_id) {
        if (module_infos[module_id].index == -1) {
            strong_connect(module_id, index, module_infos, S);
        }
    }

    std::vector<std::unordered_set<uint32_t>> sccs_set(m_modules.size());
    for (uint32_t to_id = 0; to_id < m_modules.size(); ++to_id) {
        const auto& to_module = m_modules[to_id];
        for (uint32_t from_id : m_edges[to_id]) {
            const auto& from_module = m_modules[from_id];
            if (from_module.scc_id != to_module.scc_id && sccs_set[to_module.scc_id].insert(from_module.scc_id).second) {
                m_sccs[to_module.scc_id].from_sccs.push_back(from_module.scc_id);
            }
        }
    }

    for (uint32_t scc_id = 0; scc_id < (uint32_t)m_sccs.size(); ++scc_id) {
        version_sccs(scc_id);
    }
}

void builder_ctx_t::strong_connect(uint32_t id, uint32_t& index, std::vector<module_info_t>& module_infos, std::stack<uint32_t>& S) {
    module_info_t& v = module_infos[id];
    v.index = index;
    v.lowlink = index;
    ++index;
    S.push(id);
    v.on_stack = true;

    for (uint32_t w_id : m_edges[id]) {
        module_info_t& w = module_infos[w_id];
        if (w.index == -1) {
            strong_connect(w_id, index, module_infos, S);
            v.lowlink = std::min(v.lowlink, w.lowlink);
        } else if (w.on_stack) {
            v.lowlink = std::min(v.lowlink, w.index);
        }
    }

    if (v.lowlink == v.index) {
        std::set<std::string> ordered_module_names;
        scc_t scc = {};
        while (1) {
            uint32_t w_id = S.top();
            S.pop();
            module_info_t& w = module_infos[w_id];
            w.on_stack = false;
            m_modules[w_id].scc_id = m_sccs.size();
            scc.module_ids.push_back(w_id);
            ordered_module_names.insert(m_modules[w_id].module_dir.stem().string());
            if (id == w_id) {
                break ;
            }
        }
        m_sccs.emplace_back(std::move(scc));
    }
}

void builder_ctx_t::export_libraries(uint32_t scc_id, bundle_type_t bundle_type, std::vector<bool>& visited, std::vector<std::vector<std::filesystem::path>>& library_groups) {
    if (visited[scc_id]) {
        return ;
    }
    visited[scc_id] = true;

    auto& scc = m_sccs[scc_id];

    for (auto& from_scc_id : scc.from_sccs) {
        export_libraries(from_scc_id, bundle_type, visited, library_groups);
    }

    std::vector<std::filesystem::path> library_group;

    for (uint32_t module_id : scc.module_ids) {
        builder_api_t api(module_id);
        const auto library_group_dir = api.install_dir(this, bundle_type);
        // TODO: some modules will not emit anything, need another way to cache 'builder__export_libraries' result
        if (!std::filesystem::exists(library_group_dir)) {
            const auto builder_plugin = build_builder_plugin(module_id);
            if (!std::filesystem::exists(builder_plugin)) {
                throw std::runtime_error(std::format("export_libraries: builder plugin '{}' does not exist", builder_plugin.string()));
            }

            void* builder_plugin_handle = dlopen(builder_plugin.c_str(), RTLD_NOW | RTLD_LOCAL);
            if (!builder_plugin_handle) {
                throw std::runtime_error(std::format("export_libraries: failed to load builder plugin '{}': {}", builder_plugin.string(), dlerror()));
            }

            try {
                typedef void (*builder__export_libraries_t)(builder_ctx_t* ctx, const builder_api_t* api, bundle_type_t bundle_type);
                builder__export_libraries_t builder__export_libraries = (builder__export_libraries_t) dlsym(builder_plugin_handle, "builder__export_libraries");
                if (!builder__export_libraries) {
                    throw std::runtime_error(std::format("export_libraries: failed to load symbol 'builder__export_libraries' from builder plugin '{}': {}", builder_plugin.string(), dlerror()));
                }

                builder__export_libraries(this, &api, bundle_type);
                dlclose(builder_plugin_handle);

                const auto& module = m_modules[module_id];
                std::error_code ec;
                for (const auto& entry : std::filesystem::directory_iterator(m_artifacts_dir / module.module_dir.stem(), ec)) {
                    if (!entry.is_directory()) {
                        continue ;
                    }

                    const auto& path = entry.path();
                    if (path < module.artifact_dir) {
                        std::filesystem::remove_all(path);
                    }
                }
                if (ec) {
                    throw std::runtime_error(std::format("export_libraries: failed to iterate artifacts directory '{}': {}", (m_artifacts_dir / module.module_dir.stem()).string(), ec.message()));
                }
            } catch (...) {
                dlclose(builder_plugin_handle);
                std::filesystem::remove(library_group_dir);
                throw ;
            }
        }

        if (std::filesystem::exists(library_group_dir)) {
            // TODO: recursively collect them
            std::error_code ec;
            for (const auto& entry : std::filesystem::directory_iterator(library_group_dir, ec)) {
                library_group.push_back(entry.path());
            }
            if (ec) {
                throw std::runtime_error(std::format("export_libraries: failed to iterate library group directory '{}': {}", library_group_dir.string(), ec.message()));
            }
        }
    }

    if (!library_group.empty()) {
        library_groups.emplace_back(std::move(library_group));
    }
}

void builder_ctx_t::version_sccs(uint32_t scc_id) {
    scc_t& scc = m_sccs[scc_id];
    if (scc.version != 0) {
        return ;
    }

    scc.version = m_builder_version;

    for (const auto from_scc_id : scc.from_sccs) {
        version_sccs(from_scc_id);
        scc.version = std::max(scc.version, m_sccs[from_scc_id].version);
    }

    for (const auto module_id : scc.module_ids) {
        scc.version = std::max(scc.version, m_modules[module_id].version);
    }

    for (const auto module_id : scc.module_ids) {
        m_modules[module_id].version = scc.version;
        m_modules[module_id].artifact_dir = artifact_dir(m_modules[module_id].module_dir, m_modules[module_id].version);
    }
}

void builder_ctx_t::svg_overview(const std::filesystem::path& dir, const std::string& file_name_stem) {
    if (!std::filesystem::exists(dir)) {
        // TODO: cleanup after fail
        if (!std::filesystem::create_directories(dir)) {
            throw std::runtime_error(std::format("svg_overview: failed to create directory '{}'", dir.string()));
        }
    }

    const std::string dot_file = dir / (file_name_stem + ".dot");
    const std::string svg_file = dir / (file_name_stem + ".svg");
    const std::string png_file = dir / (file_name_stem + ".png");

    std::ofstream ofs(dot_file);
    if (!ofs) {
        throw std::runtime_error("svg_overview: could not open dot file");
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

    for (uint32_t scc_id = 0; scc_id < m_sccs.size(); ++scc_id) {
        const auto& scc = m_sccs[scc_id];

        ofs << std::format(
            "    subgraph cluster_scc{} {{\n"
            "        style=rounded;\n"
            "        margin=10;\n",
            scc_id, scc_id, scc.module_ids.size()
        );

        for (uint32_t id : scc.module_ids) {
            const auto& module = m_modules[id];
            const auto name = module.module_dir.stem().string();

            const auto dot_id = std::format("n{}", id);

            ofs << std::format("        {} [label=\"{}\"];\n", dot_id, name);
        }

        ofs << "    }\n";
    }

    for (uint32_t to = 0; to < m_edges.size(); ++to) {
        for (uint32_t from : m_edges[to]) {
            const auto from_dot = std::format("n{}", from);
            const auto to_dot   = std::format("n{}", to);

            ofs << std::format("    {} -> {};\n", to_dot, from_dot);
        }
    }

    ofs << "}\n";
    ofs.close();

    if (std::system(std::format("dot -Tsvg {} > {}", dot_file, svg_file).c_str()) != 0) {
        throw std::runtime_error("svg_overview: graphviz render failed");
    }

    if (std::system(std::format("rsvg-convert {} -o {}", svg_file, png_file).c_str()) != 0) {
        throw std::runtime_error("svg_overview: could not create svg");
    }

    std::filesystem::remove(dot_file);
    // std::filesystem::remove(svg_file);
}

void builder_ctx_t::svg_sccs(const std::filesystem::path& dir, const std::string& file_name_stem) {
    if (!std::filesystem::exists(dir)) {
        // TODO: cleanup after fail
        if (!std::filesystem::create_directories(dir)) {
            throw std::runtime_error(std::format("svg_sccs: failed to create directory '{}'", dir.string()));
        }
    }
    
    const std::string dot_file = dir / (file_name_stem + ".dot");
    const std::string svg_file = dir / (file_name_stem + ".svg");
    const std::string png_file = dir / (file_name_stem + ".png");

    std::ofstream ofs(dot_file);
    if (!ofs) {
        throw std::runtime_error("svg_sccs: could not open dot file");
    }

    ofs <<
    R"(digraph SCCs {
        rankdir=LR;
        node [shape=box fontname="monospace"];
        edge [fontname="monospace"];
    )";

    std::vector<std::set<uint32_t>> dag(m_sccs.size());

    for (size_t to = 0; to < m_modules.size(); ++to) {
        uint32_t scc_to = m_modules[to].scc_id;
        for (uint32_t from : m_edges[to]) {
            uint32_t scc_from = m_modules[from].scc_id;
            if (scc_from != scc_to) {
                dag[scc_from].insert(scc_to);
            }
        }
    }

    auto reachable = [&](uint32_t src, uint32_t dst) {
        std::vector<bool> seen(m_sccs.size(), false);
        std::stack<uint32_t> st;
        st.push(src);
        seen[src] = true;

        while (!st.empty()) {
            uint32_t u = st.top(); st.pop();
            for (uint32_t v : dag[u]) {
                if (v == dst) return true;
                if (!seen[v]) {
                    seen[v] = true;
                    st.push(v);
                }
            }
        }
        return false;
    };

    std::set<std::pair<uint32_t,uint32_t>> reduced_edges;

    for (uint32_t u = 0; u < m_sccs.size(); ++u) {
        for (uint32_t v : dag[u]) {
            bool redundant = false;
            for (uint32_t w : dag[u]) {
                if (w != v && reachable(w, v)) {
                    redundant = true;
                    break;
                }
            }
            if (!redundant) {
                reduced_edges.emplace(u, v);
            }
        }
    }

    for (uint32_t scc_id = 0; scc_id < m_sccs.size(); ++scc_id) {
        const size_t sz = m_sccs[scc_id].module_ids.size();

        double w = 1.2 + 0.15 * std::log2(sz + 1);
        double h = 0.6 + 0.10 * std::log2(sz + 1);

        ofs << std::format("    scc{} [label=\"SCC {}\\n{} nodes\" width={:.2f} height={:.2f} fixedsize=true];\n", scc_id, scc_id, sz, w, h);
    }

    for (auto [u, v] : reduced_edges) {
        ofs << std::format("    scc{} -> scc{};\n", v, u);
    }

    ofs << "}\n";
    ofs.close();

    if (std::system(std::format("dot -Tsvg {} > {}", dot_file, svg_file).c_str()) != 0) {
        throw std::runtime_error("svg_sccs: graphviz render failed");
    }

    if (std::system(std::format("rsvg-convert {} -o {}", svg_file, png_file).c_str()) != 0) {
        throw std::runtime_error("svg_sccs: could not create svg");
    }

    std::filesystem::remove(dot_file);
    // std::filesystem::remove(svg_file);
}

void builder_ctx_t::svg_scc(const std::filesystem::path& dir, uint32_t scc_id, const std::string& file_name_stem) {
    if (!std::filesystem::exists(dir)) {
        // TODO: cleanup after fail
        if (!std::filesystem::create_directories(dir)) {
            throw std::runtime_error(std::format("svg_scc: failed to create directory '{}'", dir.string()));
        }
    }

    if (m_sccs.size() <= scc_id) {
        throw std::runtime_error("svg_scc: invalid SCC id");
    }

    const auto& scc = m_sccs[scc_id];

    const std::string dot_file = dir / (file_name_stem + ".dot");
    const std::string svg_file = dir / (file_name_stem + ".svg");
    const std::string png_file = dir / (file_name_stem + ".png");

    std::ofstream ofs(dot_file);
    if (!ofs)
        throw std::runtime_error("svg_scc: could not open dot file");

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

    if (scc.module_ids.size() == 1) {
        const auto& module = m_modules[scc.module_ids[0]];
        ofs << std::format("    {};\n", module.module_dir.stem().string());
    } else {
        for (uint32_t to : scc.module_ids) {
            const auto& to_module = m_modules[to];
            const auto to_name = to_module.module_dir.stem().string();
            for (uint32_t from : m_edges[to]) {
                const auto& from_module = m_modules[from];
                const auto from_name = from_module.module_dir.stem().string();
                if (from_module.scc_id == scc_id) {
                    ofs << std::format("    {} -> {};\n", to_name, from_name);
                }
            }
        }
    }

    ofs << "}\n";
    ofs.close();

    if (std::system(std::format("neato -Tsvg {} > {}", dot_file, svg_file).c_str()) != 0) {
        throw std::runtime_error("svg_scc: graphviz render failed");
    }

    if (std::system(std::format("rsvg-convert {} -o {}", svg_file, png_file).c_str()) != 0) {
        throw std::runtime_error("svg_scc: could not create svg");
    }

    std::filesystem::remove(dot_file);
    // std::filesystem::remove(svg_file);
}
