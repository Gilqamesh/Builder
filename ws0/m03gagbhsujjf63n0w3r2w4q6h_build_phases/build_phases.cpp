#include "build_phases.h"

#include <m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain/cxx_toolchain.h>
#include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
#include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>
#include <m03gagbhsvr0m5w15urj0o291m_process/process.h>
#include <m03gagbhsyhlx2pk5sdabbr1sx_signal_handler/signal_handler.h>
#include <m03gagbhsx4j5z28bqkac3dhhh_shared_library/shared_library.h>
#include <m03gn8rf3pe86v64vphnaam6rl_source_dependencies/source_dependencies.h>
#include <m03gn8rf3pe8dkpk1uwsemhhmd_artifact_store/artifact_store.h>
#include <m03gn8rf3peew0re4l1s2vvaw6_bootstrap_seed/bootstrap_seed.h>

#include <algorithm>
#include <cstdint>
#include <format>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace m03gagbhsujjf63n0w3r2w4q6h_build_phases {

namespace artifact_store = m03gn8rf3pe8dkpk1uwsemhhmd_artifact_store;
namespace bootstrap_seed = m03gn8rf3peew0re4l1s2vvaw6_bootstrap_seed;
namespace filesystem = m03gagbhsnusi43zogoacgj2ez_filesystem;
namespace source_dependencies = m03gn8rf3pe86v64vphnaam6rl_source_dependencies;
namespace workspace_graph = m03gagbhsp2drqq3gkop8pzfrm_workspace_graph;

using module_list_t = std::vector<workspace_graph::module_t*>;

struct module_ptr_less_t {
    bool operator()(const workspace_graph::module_t* a, const workspace_graph::module_t* b) const {
        if (a == b) {
            return false;
        }
        if (a->workspace() == b->workspace()) {
            return a->name() < b->name();
        }
        return a->workspace() < b->workspace();
    }
};

using module_set_t = std::set<workspace_graph::module_t*, module_ptr_less_t>;
using module_dependency_map_t = std::map<workspace_graph::module_t*, module_list_t, module_ptr_less_t>;
using module_library_root_map_t = std::map<workspace_graph::module_t*, filesystem::path_t, module_ptr_less_t>;

static std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::path_t> include_dirs_from_outputs(const std::vector<interface_phase_t::installed_t>& interfaces) {
    std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::path_t> include_dirs;
    include_dirs.reserve(interfaces.size());

    for (const auto& interface_output : interfaces) {
        include_dirs.push_back(interface_output.root());
    }

    return include_dirs;
}

static std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t> compiler_source_files(const std::vector<phase_base_t::built_t>& source_files) {
    std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t> result;
    result.reserve(source_files.size());

    for (const auto& source_file : source_files) {
        result.push_back(source_file.rooted_path());
    }

    return result;
}

static bool is_library_compile_source_file(const filesystem::rooted_path_t& source_file) {
    const auto path = source_file.path();
    const auto relative_path = source_file.relative_path().string();

    if (path.filename() == workspace_graph::BUILDER_CPP || path.filename() == workspace_graph::CLI_CPP) {
        return false;
    }
    if (relative_path.starts_with("cli/") || relative_path.starts_with("test/")) {
        return false;
    }

    return true;
}

static std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t> library_compile_source_files(
    const std::vector<phase_base_t::built_t>& source_files
) {
    std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t> result;

    for (const auto& source_file : compiler_source_files(source_files)) {
        if (is_library_compile_source_file(source_file)) {
            result.push_back(source_file);
        }
    }

    return result;
}

static std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t> appended_unique(
    std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t> result,
    const std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t>& files
) {
    for (const auto& file : files) {
        const auto already_present = std::find_if(result.begin(), result.end(), [&](const auto& existing) {
            return existing.root() == file.root() && existing.relative_path() == file.relative_path();
        }) != result.end();

        if (!already_present) {
            result.push_back(file);
        }
    }

    return result;
}

static std::string cxx_string_literal(std::string_view value) {
    std::string result("\"");

    for (const char c : value) {
        if (c == '\\' || c == '"') {
            result.push_back('\\');
        }
        result.push_back(c);
    }

    result.push_back('"');
    return result;
}

static m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t module_library_relative_output_path(
    const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t& module_name
) {
    return m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(std::format("lib{}.so", module_name));
}

static m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t binary_target_relative_output_path(std::string_view target) {
    if (target.empty()) {
        throw std::runtime_error("m03gagbhsujjf63n0w3r2w4q6h_build_phases::binary_target_relative_output_path: target is empty");
    }
    if (target.find_first_of("/\\") != std::string_view::npos) {
        throw std::runtime_error(std::format("m03gagbhsujjf63n0w3r2w4q6h_build_phases::binary_target_relative_output_path: target '{}' contains a path separator", target));
    }

    return m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(std::string(target));
}

static m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t binary_runtime_artifact_relative_output_path(
    const phase_base_t::built_t& artifact
) {
    return m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(
        artifact.rooted_path().path().filename());
}

struct hash_t {
    std::uint64_t value = 1469598103934665603ULL;

    void add(std::string_view data) {
        for (const unsigned char c : data) {
            value ^= c;
            value *= 1099511628211ULL;
        }
        value ^= 0xff;
        value *= 1099511628211ULL;
    }

    void add(std::uint64_t data) {
        for (std::size_t i = 0; i < sizeof(data); ++i) {
            value ^= static_cast<unsigned char>((data >> (i * 8)) & 0xff);
            value *= 1099511628211ULL;
        }
        value ^= 0xfe;
        value *= 1099511628211ULL;
    }

    std::string string() const {
        return std::format("{:016x}", value);
    }
};

static std::uint64_t file_time_version(const filesystem::path_t& path) {
    return workspace_graph::version_t(filesystem::last_write_time(path)).value;
}

static void hash_file_version(hash_t& hash, const filesystem::rooted_path_t& file) {
    hash.add(file.relative_path().string());
    hash.add(file_time_version(file.path()));
}

static std::vector<filesystem::rooted_path_t> sorted_files(std::vector<filesystem::rooted_path_t> files) {
    std::sort(files.begin(), files.end(), [](const auto& a, const auto& b) {
        if (a.root() == b.root()) {
            return a.relative_path().string() < b.relative_path().string();
        }
        return a.root().string() < b.root().string();
    });
    return files;
}

static std::vector<filesystem::rooted_path_t> library_dependency_source_files(const workspace_graph::module_t& module) {
    return source_dependencies::library_source_files(module.source_dir());
}

static std::vector<filesystem::rooted_path_t> public_api_validation_source_files(const workspace_graph::module_t& module) {
    const auto relative_path = filesystem::relative_path_t("test/public_api.cpp");
    const auto source_root = module.source_dir();
    const auto source_path = source_root / relative_path;

    if (!filesystem::exists(source_path)) {
        return {};
    }

    return {
        filesystem::rooted_path_t(source_root, relative_path)
    };
}

static module_list_t direct_library_dependencies(
    workspace_graph::module_t& module,
    module_dependency_map_t& direct_dependencies_by_module
) {
    if (const auto it = direct_dependencies_by_module.find(&module); it != direct_dependencies_by_module.end()) {
        return it->second;
    }

    const auto source_files = library_dependency_source_files(module);
    auto dependencies = source_dependencies::scan_sources(
        module,
        source_files,
        &module,
        source_dependencies::dependency_mode_t::MODULE
    ).dependencies;

    auto [it, inserted] = direct_dependencies_by_module.emplace(&module, std::move(dependencies));
    (void)inserted;
    return it->second;
}

static bool contains_module(const module_list_t& modules, const workspace_graph::module_t& module) {
    return std::find(modules.begin(), modules.end(), &module) != modules.end();
}

struct library_scc_search_t {
    workspace_graph::module_t& root;
    module_dependency_map_t& direct_dependencies_by_module;
    std::map<workspace_graph::module_t*, std::size_t, module_ptr_less_t> index_by_module;
    std::map<workspace_graph::module_t*, std::size_t, module_ptr_less_t> lowlink_by_module;
    std::vector<workspace_graph::module_t*> stack;
    module_set_t on_stack;
    module_list_t root_scc;
    std::size_t next_index = 0;

    void visit(workspace_graph::module_t& module) {
        index_by_module.emplace(&module, next_index);
        lowlink_by_module.emplace(&module, next_index);
        ++next_index;
        stack.push_back(&module);
        on_stack.insert(&module);

        for (auto* dependency : direct_library_dependencies(module, direct_dependencies_by_module)) {
            if (!index_by_module.contains(dependency)) {
                visit(*dependency);
                lowlink_by_module[&module] = std::min(
                    lowlink_by_module[&module],
                    lowlink_by_module[dependency]
                );
            } else if (on_stack.contains(dependency)) {
                lowlink_by_module[&module] = std::min(
                    lowlink_by_module[&module],
                    index_by_module[dependency]
                );
            }
        }

        if (lowlink_by_module[&module] != index_by_module[&module]) {
            return ;
        }

        module_list_t component;
        while (!stack.empty()) {
            auto* member = stack.back();
            stack.pop_back();
            on_stack.erase(member);
            component.push_back(member);
            if (member == &module) {
                break;
            }
        }

        if (contains_module(component, root)) {
            root_scc = std::move(component);
        }
    }
};

static module_list_t library_scc_modules(
    workspace_graph::module_t& module,
    module_dependency_map_t& direct_dependencies_by_module
) {
    library_scc_search_t search {
        .root = module,
        .direct_dependencies_by_module = direct_dependencies_by_module
    };
    search.visit(module);
    std::sort(search.root_scc.begin(), search.root_scc.end(), module_ptr_less_t());
    return search.root_scc;
}

struct source_set_dependencies_t {
    std::vector<interface_phase_t::installed_t> interfaces;
    m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::link_inputs_t link_inputs;
    module_list_t modules;
};

static std::vector<interface_phase_t::installed_t> install_interfaces_from_modules(
    const module_list_t& modules
) {
    std::vector<interface_phase_t::installed_t> result;

    for (auto* module : modules) {
        const auto phase = phase_base_t::make(*module);
        result.push_back(phase->install<interface_phase_t>());
    }

    return result;
}

static void append_libraries_from_root(
    m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::link_inputs_t& result,
    const filesystem::path_t& root
) {
    if (!filesystem::exists(root)) {
        return ;
    }

    for (const auto& library : filesystem::find(root, !filesystem::find_include_predicate_t::is_dir, filesystem::find_descend_predicate_t::descend_all)) {
        result.libraries.push_back(library.path());
    }
}

static m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::link_inputs_t link_inputs_from_modules(
    const module_list_t& modules
) {
    m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::link_inputs_t result;
    for (auto* module : modules) {
        const auto phase = phase_base_t::make(*module);
        const auto libraries = phase->install<library_phase_t>();
        append_libraries_from_root(result, libraries.root());
    }

    return result;
}

static m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::link_inputs_t link_inputs_from_modules(
    const module_list_t& modules,
    const module_library_root_map_t& staged_library_roots
) {
    m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::link_inputs_t result;
    for (auto* module : modules) {
        if (const auto staged_it = staged_library_roots.find(module); staged_it != staged_library_roots.end()) {
            append_libraries_from_root(result, staged_it->second);
            continue;
        }

        const auto phase = phase_base_t::make(*module);
        const auto libraries = phase->install<library_phase_t>();
        append_libraries_from_root(result, libraries.root());
    }

    return result;
}

static source_set_dependencies_t install_source_set_dependencies(
    const workspace_graph::module_t& module,
    const std::vector<filesystem::rooted_path_t>& source_files,
    bool include_owner,
    source_dependencies::dependency_mode_t dependency_mode,
    bool link_libraries
) {
    const auto modules = source_dependencies::dependency_modules(
        module,
        source_files,
        include_owner,
        dependency_mode,
        library_dependency_source_files
    );
    source_set_dependencies_t result {
        .interfaces = install_interfaces_from_modules(modules),
        .link_inputs = {},
        .modules = modules
    };

    if (link_libraries) {
        result.link_inputs = link_inputs_from_modules(modules);
    }

    return result;
}

static void hash_file_versions(hash_t& hash, const std::vector<filesystem::rooted_path_t>& files) {
    for (const auto& file : sorted_files(files)) {
        hash_file_version(hash, file);
    }
}

static void hash_defines(
    hash_t& hash,
    const std::vector<m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::define_t>& defines
) {
    for (const auto& define : defines) {
        hash.add(define.key());
        hash.add(define.value());
    }
}

static std::string source_phase_cache_key(const workspace_graph::module_t& module);

static void hash_modules(hash_t& hash, const module_list_t& modules) {
    for (const auto* module : modules) {
        hash.add(module->name().unique_name());
        hash.add(source_phase_cache_key(*module));
    }
}

static std::string source_set_cache_key(
    std::string_view kind,
    const workspace_graph::module_t& module,
    std::string_view name,
    const std::vector<filesystem::rooted_path_t>& source_files,
    const std::vector<m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::define_t>& defines,
    source_dependencies::dependency_mode_t dependency_mode,
    const source_set_dependencies_t& dependencies
) {
    hash_t hash;
    hash.add("source-set-cache-v4");
    hash.add(kind);
    hash.add(module.name().unique_name());
    hash.add(name);

    const auto scan = source_dependencies::scan_sources(module, source_files, nullptr, dependency_mode);
    hash_file_versions(hash, scan.local_files);
    hash_defines(hash, defines);
    hash_modules(hash, dependencies.modules);

    return hash.string();
}

static std::string library_phase_cache_key(const workspace_graph::module_t& module) {
    const auto source_files = library_dependency_source_files(module);
    const auto cache_source_files = appended_unique(
        source_files,
        public_api_validation_source_files(module)
    );
    const auto modules = source_dependencies::dependency_modules(
        module,
        cache_source_files,
        false,
        source_dependencies::dependency_mode_t::MODULE,
        library_dependency_source_files
    );

    hash_t hash;
    hash.add("library-phase-scc-validation-v5");
    hash.add(module.name().unique_name());
    hash.add(source_phase_cache_key(module));
    hash_file_versions(hash, cache_source_files);
    hash_modules(hash, modules);
    return hash.string();
}

static std::string source_phase_cache_key(const workspace_graph::module_t& module) {
    hash_t hash;
    hash.add("source-phase-version-v1");
    hash.add(module.name().unique_name());
    hash.add(module.version().value);
    return hash.string();
}

static std::string interface_phase_cache_key(const workspace_graph::module_t& module) {
    hash_t hash;
    hash.add("interface-phase");
    hash.add(module.name().unique_name());
    hash.add(source_phase_cache_key(module));
    return hash.string();
}

static filesystem::path_t phase_artifact_dir(
    const workspace_graph::module_t& module,
    std::string_view phase_name,
    std::string_view hash
) {
    return artifact_store::artifact_dir(module, filesystem::relative_path_t(std::string(phase_name)), hash);
}

static filesystem::relative_path_t binary_target_kind(std::string_view target) {
    return filesystem::relative_path_t("binary") / binary_target_relative_output_path(target);
}

static filesystem::path_t binary_target_artifact_dir(
    const workspace_graph::module_t& module,
    std::string_view target,
    std::string_view hash
) {
    return artifact_store::artifact_dir(module, binary_target_kind(target), hash);
}

static void update_latest_phase(
    const workspace_graph::module_t& module,
    std::string_view phase_name,
    const filesystem::path_t& artifact_dir
) {
    artifact_store::update_latest(module, filesystem::relative_path_t(std::string(phase_name)), artifact_dir);
}

static void update_latest_binary_target(
    const workspace_graph::module_t& module,
    std::string_view target,
    const filesystem::path_t& artifact_dir
) {
    artifact_store::update_latest(module, binary_target_kind(target), artifact_dir);
}

phase_base_t::built_t::built_t(const m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t& rooted_path):
    m_rooted_path(rooted_path)
{
}

const m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t& phase_base_t::built_t::rooted_path() const {
    return m_rooted_path;
}

phase_base_t::phase_base_t(
    std::string_view name,
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module,
    std::unique_ptr<phase_base_t> previous_phase
):
    m_name(name),
    m_module(module),
    m_previous_phase(std::move(previous_phase)),
    m_artifact_dir(std::nullopt)
{
}

std::unique_ptr<phase_base_t> phase_base_t::make(
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module
) {
    return make(module, "");
}

std::unique_ptr<phase_base_t> phase_base_t::make(
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module,
    std::string_view target
) {
    std::unique_ptr<phase_base_t> phase;
    phase = std::make_unique<source_phase_t>(module, std::move(phase));
    phase = std::make_unique<interface_phase_t>(module, std::move(phase));
    phase = std::make_unique<library_phase_t>(module, std::move(phase));
    phase = std::make_unique<binary_phase_t>(module, target, std::move(phase));
    return phase;
}

std::string_view phase_base_t::name() const {
    return m_name;
}

m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& phase_base_t::module() const {
    return m_module;
}

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t phase_base_t::artifact_dir() const {
    if (!m_artifact_dir) {
        throw std::runtime_error(std::format("m03gagbhsujjf63n0w3r2w4q6h_build_phases::phase_base_t::artifact_dir: phase '{}' artifact directory has not been resolved", name()));
    }

    return *m_artifact_dir;
}

void phase_base_t::artifact_dir(m03gagbhsnusi43zogoacgj2ez_filesystem::path_t artifact_dir) const {
    m_artifact_dir = std::move(artifact_dir);
}

bool phase_base_t::has_artifact_dir() const {
    return m_artifact_dir.has_value();
}

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t phase_base_t::build_dir() const {
    return artifact_dir() / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("build");
}

phase_base_t::built_t phase_base_t::build(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& path) const {
    for (const auto* phase = previous_phase(); phase != nullptr; phase = phase->previous_phase()) {
        if (!phase->has_artifact_dir()) {
            continue;
        }
        const auto previous_install_dir = phase->install_dir();
        if (previous_install_dir.is_child(path)) {
            return build(m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t(previous_install_dir, previous_install_dir.relative(path)));
        }
    }

    throw std::runtime_error(std::format(
        "m03gagbhsujjf63n0w3r2w4q6h_build_phases::phase_base_t::build: build input '{}' is not under an earlier phase install_dir for phase '{}'",
        path,
        name()
    ));
}

phase_base_t::built_t phase_base_t::build(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& external, const m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t& as) const {
    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(external)) {
        throw std::runtime_error(std::format("m03gagbhsujjf63n0w3r2w4q6h_build_phases::phase_base_t::build: external build input '{}' does not exist", external));
    }

    const auto target_path = build_dir() / as;
    if (m03gagbhsnusi43zogoacgj2ez_filesystem::exists(target_path)) {
        throw std::runtime_error(std::format("m03gagbhsujjf63n0w3r2w4q6h_build_phases::phase_base_t::build: target build path '{}' already exists", target_path));
    }

    m03gagbhsnusi43zogoacgj2ez_filesystem::create_symlink(external, target_path);

    return built_t(m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t(build_dir(), as));
}

phase_base_t::built_t phase_base_t::build(const m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t& rooted_path) const {
    for (const auto* phase = previous_phase(); phase != nullptr; phase = phase->previous_phase()) {
        if (!phase->has_artifact_dir()) {
            continue;
        }
        if (rooted_path.root() == phase->install_dir()) {
            return built_t(rooted_path);
        }
    }

    throw std::runtime_error(std::format(
        "m03gagbhsujjf63n0w3r2w4q6h_build_phases::phase_base_t::build: selected input root '{}' is not an installed root of a previous phase for phase '{}'",
        rooted_path.root(),
        name()
    ));
}

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t phase_base_t::install_dir() const {
    return artifact_dir() / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("install");
}

void phase_base_t::install_as(
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& path,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t& relative_path
) const {
    const auto installed_artifact = install_dir() / relative_path;

    if (m03gagbhsnusi43zogoacgj2ez_filesystem::exists(installed_artifact)) {
        throw std::runtime_error(std::format("m03gagbhsujjf63n0w3r2w4q6h_build_phases::phase_base_t::install_as: phase '{}' already has an artifact at relative path '{}'", name(), install_dir().relative(installed_artifact)));
    }

    m03gagbhsnusi43zogoacgj2ez_filesystem::copy(path, installed_artifact);
}

const phase_base_t* phase_base_t::previous_phase() const {
    return m_previous_phase.get();
}

m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t phase_base_t::installed_relative_path(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& path) const {
    const auto build_dir = this->build_dir();
    if (build_dir.is_child(path)) {
        return build_dir.relative(path);
    }

    for (const auto* phase = previous_phase(); phase != nullptr; phase = phase->previous_phase()) {
        if (!phase->has_artifact_dir()) {
            continue;
        }
        const auto previous_install_dir = phase->install_dir();
        if (previous_install_dir.is_child(path)) {
            return previous_install_dir.relative(path);
        }
    }

    throw std::runtime_error(std::format(
        "m03gagbhsujjf63n0w3r2w4q6h_build_phases::phase_base_t::installed_relative_path: artifact '{}' is not under phase '{}' build_dir or an earlier phase install_dir",
        path,
        name()
    ));
}

void phase_base_t::install(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& path) const {
    install_as(path, installed_relative_path(path));
}

void phase_base_t::install(const m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t& rooted_path) const {
    if (rooted_path.root() == build_dir()) {
        install_as(rooted_path.path(), rooted_path.relative_path());
        return ;
    }

    for (const auto* phase = previous_phase(); phase != nullptr; phase = phase->previous_phase()) {
        if (!phase->has_artifact_dir()) {
            continue;
        }
        if (rooted_path.root() == phase->install_dir()) {
            install_as(rooted_path.path(), rooted_path.relative_path());
            return ;
        }
    }

    throw std::runtime_error(std::format(
        "m03gagbhsujjf63n0w3r2w4q6h_build_phases::phase_base_t::install: selected path root '{}' is not phase '{}' build_dir or an earlier phase install_dir",
        rooted_path.root(),
        name()
    ));
}

void phase_base_t::install(const built_t& built) const {
    install(built.rooted_path());
}

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t phase_base_t::builder_plugin() const {
    if (bootstrap_seed::is_module(m_module)) {
        return bootstrap_seed::builder_plugin_path(m_module.workspace().graph());
    }

    const std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t> source_files {
        m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t(
            m_module.source_dir(),
            m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::BUILDER_CPP)
        )
    };
    const auto dependencies = install_source_set_dependencies(
        m_module,
        source_files,
        false,
        source_dependencies::dependency_mode_t::BUILDER,
        true
    );
    const auto cache_key = source_set_cache_key(
        "builder-plugin",
        m_module,
        "builder",
        source_files,
        {},
        source_dependencies::dependency_mode_t::BUILDER,
        dependencies
    );
    const auto artifact_dir = phase_artifact_dir(m_module, "builder", cache_key);
    const auto build_dir = artifact_dir / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("build");
    const auto install_dir = artifact_dir / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("install");
    const auto plugin_path = install_dir / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("builder.so");
    const auto started = artifact_store::started_marker(artifact_dir);
    const auto complete = artifact_store::completed_marker(artifact_dir);

    if (m03gagbhsnusi43zogoacgj2ez_filesystem::exists(complete)) {
        if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(plugin_path)) {
            throw std::runtime_error(std::format("m03gagbhsujjf63n0w3r2w4q6h_build_phases::phase_base_t::builder_plugin: completed builder plugin '{}' does not exist", plugin_path));
        }

        update_latest_phase(m_module, "builder", artifact_dir);
        return plugin_path;
    }

    if (m03gagbhsnusi43zogoacgj2ez_filesystem::exists(started)) {
        throw std::runtime_error(std::format("m03gagbhsujjf63n0w3r2w4q6h_build_phases::phase_base_t::builder_plugin: re-entry detected for builder plugin '{}'", plugin_path));
    }

    if (m03gagbhsnusi43zogoacgj2ez_filesystem::exists(build_dir)) {
        m03gagbhsnusi43zogoacgj2ez_filesystem::remove_all(build_dir);
    }
    if (m03gagbhsnusi43zogoacgj2ez_filesystem::exists(install_dir)) {
        m03gagbhsnusi43zogoacgj2ez_filesystem::remove_all(install_dir);
    }

    try {
        {
            m03gagbhsyhlx2pk5sdabbr1sx_signal_handler::scoped_termination_guard_t termination_guard;

            m03gagbhsnusi43zogoacgj2ez_filesystem::create_directories(build_dir);
            m03gagbhsnusi43zogoacgj2ez_filesystem::touch(started);

            m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::build_library(
                build_dir,
                include_dirs_from_outputs(dependencies.interfaces),
                source_files,
                {},
                dependencies.link_inputs,
                plugin_path
            );
        }

        if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(plugin_path)) {
            throw std::runtime_error(std::format("m03gagbhsujjf63n0w3r2w4q6h_build_phases::phase_base_t::builder_plugin: expected builder plugin '{}' to exist", plugin_path));
        }

        m03gagbhsnusi43zogoacgj2ez_filesystem::touch(complete);
        m03gagbhsnusi43zogoacgj2ez_filesystem::remove(started);
        update_latest_phase(m_module, "builder", artifact_dir);

        return plugin_path;
    } catch (...) {
        m03gagbhsnusi43zogoacgj2ez_filesystem::remove_all(artifact_dir);
        throw ;
    }
}

source_phase_t::source_phase_t(
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module,
    std::unique_ptr<phase_base_t> previous_phase
):
    phase_base_t("source", module, std::move(previous_phase))
{
}

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t source_phase_t::source_dir() const {
    return module().source_dir();
}

source_phase_t::installed_t::installed_t(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& root):
    m_root(root)
{
}

const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& source_phase_t::installed_t::root() const {
    return m_root;
}

phase_base_t::built_t phase_base_t::source(std::string_view relative_path) const {
    const auto sources = install<source_phase_t>();
    return build(m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t(
        sources.root(),
        m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(std::string(relative_path))
    ));
}

void source_phase_t::install_source_tree() const {
    const auto source_root = source_dir();

    for (const auto& source : m03gagbhsnusi43zogoacgj2ez_filesystem::find(source_root, !m03gagbhsnusi43zogoacgj2ez_filesystem::find_include_predicate_t::is_dir, m03gagbhsnusi43zogoacgj2ez_filesystem::find_descend_predicate_t::descend_all)) {
        install_as(source.path(), source.relative_path());
    }
}

void source_phase_t::install_source(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& source) const {
    install(source);
}

interface_phase_t::interface_phase_t(
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module,
    std::unique_ptr<phase_base_t> previous_phase
):
    phase_base_t("interface", module, std::move(previous_phase))
{
}

interface_phase_t::installed_t::installed_t(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& root):
    m_root(root)
{
}

const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& interface_phase_t::installed_t::root() const {
    return m_root;
}

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t interface_phase_t::build_interface_as(
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& source,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t& relative_path
) const {
    const auto built_source = build(source);
    const auto staged_interface = build_dir() / relative_path;
    m03gagbhsnusi43zogoacgj2ez_filesystem::copy(built_source.rooted_path().path(), staged_interface);
    return staged_interface;
}

void interface_phase_t::install_headers_from_source() const {
    const auto sources = install<source_phase_t>();

    for (const auto& artifact : m03gagbhsnusi43zogoacgj2ez_filesystem::find(
        sources.root(),
        m03gagbhsnusi43zogoacgj2ez_filesystem::find_include_predicate_t::h_file || m03gagbhsnusi43zogoacgj2ez_filesystem::find_include_predicate_t::hpp_file,
        m03gagbhsnusi43zogoacgj2ez_filesystem::find_descend_predicate_t::descend_all
    )) {
        install_interface(artifact);
    }
}

void interface_phase_t::install_interface(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& interface) const {
    const auto relative_path = installed_relative_path(interface);
    const auto canonical_path = install_dir()
        / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(module().name().unique_name())
        / relative_path;
    install_as(interface, install_dir().relative(canonical_path));
}

void interface_phase_t::install_interface(const m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t& interface) const {
    install_interface(interface.path());
}

library_phase_t::library_phase_t(
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module,
    std::unique_ptr<phase_base_t> previous_phase
):
    phase_base_t("library", module, std::move(previous_phase))
{
}

library_phase_t::installed_t::installed_t(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& root):
    m_root(root)
{
}

const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& library_phase_t::installed_t::root() const {
    return m_root;
}

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t library_phase_t::build_library(
    const std::vector<phase_base_t::built_t>& source_files,
    const std::vector<m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::define_t>& defines
) const {
    const auto source_file_paths = library_compile_source_files(source_files);
    const auto dependency_source_file_paths = appended_unique(
        source_file_paths,
        library_dependency_source_files(module())
    );
    const auto dependencies = install_source_set_dependencies(
        module(),
        dependency_source_file_paths,
        false,
        source_dependencies::dependency_mode_t::MODULE,
        false
    );
    auto interfaces = std::vector<interface_phase_t::installed_t> {
        install<interface_phase_t>()
    };
    interfaces.insert(interfaces.end(), dependencies.interfaces.begin(), dependencies.interfaces.end());
    const auto relative_output_path = module_library_relative_output_path(module().name());
    const auto library = build_dir() / relative_output_path;

    try {
        const auto result = m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::build_library(
            build_dir(),
            include_dirs_from_outputs(interfaces),
            source_file_paths,
            defines,
            m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::link_inputs_t {},
            library
        );
        return result;
    } catch (...) {
        m03gagbhsnusi43zogoacgj2ez_filesystem::remove_all(build_dir());
        throw ;
    }
}

void library_phase_t::install_library(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& library) const {
    try {
        install(library);
    } catch (const std::runtime_error&) {
        install_as(library, m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(library.filename()));
    }
}

void library_phase_t::install_library(const phase_base_t::built_t& library) const {
    install(library);
}

void library_phase_t::validate_library(
    std::string_view name,
    const std::vector<phase_base_t::built_t>& source_files,
    const std::vector<m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::define_t>& defines,
    const std::vector<std::string>& arguments
) const {
    (void)binary_target_relative_output_path(name);
    if (source_files.empty()) {
        throw std::runtime_error(std::format("m03gagbhsujjf63n0w3r2w4q6h_build_phases::library_phase_t::validate_library: validation '{}' has no source files", name));
    }

    m_validations.push_back(validation_t {
        .name = std::string(name),
        .source_files = source_files,
        .defines = defines,
        .arguments = arguments
    });
}

const std::vector<library_phase_t::validation_t>& library_phase_t::validations() const {
    return m_validations;
}

binary_phase_t::binary_phase_t(
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module,
    std::string_view target,
    std::unique_ptr<phase_base_t> previous_phase
):
    phase_base_t("binary", module, std::move(previous_phase)),
    m_target(target)
{
}

binary_phase_t::installed_t::installed_t(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& root):
    m_root(root)
{
}

const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& binary_phase_t::installed_t::root() const {
    return m_root;
}

bool binary_phase_t::should_install_target(std::string_view target) const {
    return m_target.empty() || m_target == target;
}

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t binary_phase_t::installed_t::target(std::string_view target) const {
    const auto relative_output_path = binary_target_relative_output_path(target);
    const auto binary = m_root / relative_output_path / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("install") / relative_output_path;
    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(binary) || !m03gagbhsnusi43zogoacgj2ez_filesystem::is_regular_file(binary)) {
        throw std::runtime_error(std::format("m03gagbhsujjf63n0w3r2w4q6h_build_phases::binary_phase_t::installed_t::target: binary phase did not publish '{}' target at {}", target, binary));
    }

    return binary;
}

void binary_phase_t::install_binary(
    std::string_view target,
    const std::vector<phase_base_t::built_t>& source_files,
    const std::vector<m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::define_t>& defines,
    const std::vector<phase_base_t::built_t>& runtime_artifacts
) const {
    if (!should_install_target(target)) {
        return ;
    }

    const auto relative_output_path = binary_target_relative_output_path(target);
    const auto source_file_paths = compiler_source_files(source_files);
    const auto dependencies = install_source_set_dependencies(
        module(),
        source_file_paths,
        true,
        source_dependencies::dependency_mode_t::MODULE,
        true
    );
    const auto source_cache_key = source_set_cache_key(
        "binary-target",
        module(),
        target,
        source_file_paths,
        defines,
        source_dependencies::dependency_mode_t::MODULE,
        dependencies
    );
    std::string cache_key = source_cache_key;
    if (!runtime_artifacts.empty()) {
        hash_t artifact_hash;
        artifact_hash.add("binary-target-artifacts-v1");
        artifact_hash.add(source_cache_key);
        for (const auto& artifact : runtime_artifacts) {
            artifact_hash.add(binary_runtime_artifact_relative_output_path(artifact).string());
            const auto& source = artifact.rooted_path();
            if (m03gagbhsnusi43zogoacgj2ez_filesystem::is_directory(source.path())) {
                hash_file_versions(artifact_hash, m03gagbhsnusi43zogoacgj2ez_filesystem::find(
                    source.path(),
                    !m03gagbhsnusi43zogoacgj2ez_filesystem::find_include_predicate_t::is_dir,
                    m03gagbhsnusi43zogoacgj2ez_filesystem::find_descend_predicate_t::descend_all));
            } else if (m03gagbhsnusi43zogoacgj2ez_filesystem::is_regular_file(source.path())) {
                hash_file_version(artifact_hash, source);
            } else {
                throw std::runtime_error(std::format(
                    "m03gagbhsujjf63n0w3r2w4q6h_build_phases::binary_phase_t::install_binary: runtime artifact '{}' does not exist as a regular file or directory",
                    source.path()));
            }
        }
        cache_key = artifact_hash.string();
    }
    const auto artifact_dir = binary_target_artifact_dir(module(), target, cache_key);
    const auto target_build_dir = artifact_dir / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("build");
    const auto target_install_dir = artifact_dir / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("install");
    const auto binary = target_install_dir / relative_output_path;
    const auto started = artifact_store::started_marker(artifact_dir);
    const auto complete = artifact_store::completed_marker(artifact_dir);

    if (m03gagbhsnusi43zogoacgj2ez_filesystem::exists(complete)) {
        if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(binary)) {
            throw std::runtime_error(std::format("m03gagbhsujjf63n0w3r2w4q6h_build_phases::binary_phase_t::install_binary: completed target binary '{}' does not exist", binary));
        }
        for (const auto& artifact : runtime_artifacts) {
            const auto installed_artifact = target_install_dir
                / binary_runtime_artifact_relative_output_path(artifact);
            if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(installed_artifact)) {
                throw std::runtime_error(std::format(
                    "m03gagbhsujjf63n0w3r2w4q6h_build_phases::binary_phase_t::install_binary: completed target runtime artifact '{}' does not exist",
                    installed_artifact));
            }
        }

        update_latest_binary_target(module(), target, artifact_dir);
        return ;
    }

    if (m03gagbhsnusi43zogoacgj2ez_filesystem::exists(started)) {
        throw std::runtime_error(std::format("m03gagbhsujjf63n0w3r2w4q6h_build_phases::binary_phase_t::install_binary: re-entry detected for target binary '{}'", binary));
    }

    if (artifact_store::exists_or_symlink(artifact_dir)) {
        artifact_store::remove_existing_path(artifact_dir);
    }

    try {
        m03gagbhsnusi43zogoacgj2ez_filesystem::create_directories(target_build_dir);
        m03gagbhsnusi43zogoacgj2ez_filesystem::create_directories(target_install_dir);
        m03gagbhsnusi43zogoacgj2ez_filesystem::touch(started);

        m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::build_binary(
            target_build_dir,
            include_dirs_from_outputs(dependencies.interfaces),
            source_file_paths,
            defines,
            dependencies.link_inputs,
            binary
        );

        for (const auto& artifact : runtime_artifacts) {
            m03gagbhsnusi43zogoacgj2ez_filesystem::copy(
                artifact.rooted_path().path(),
                target_install_dir / binary_runtime_artifact_relative_output_path(artifact));
        }

        m03gagbhsnusi43zogoacgj2ez_filesystem::touch(complete);
        m03gagbhsnusi43zogoacgj2ez_filesystem::remove(started);
        update_latest_binary_target(module(), target, artifact_dir);
    } catch (...) {
        m03gagbhsnusi43zogoacgj2ez_filesystem::remove_all(artifact_dir);
        throw ;
    }
}

static bool is_default_library_source_file(const filesystem::rooted_path_t& source_file) {
    if (!is_library_compile_source_file(source_file)) {
        return false;
    }

    const auto extension = source_file.relative_path().extension();
    return extension == ".c" || extension == ".cpp";
}

static void install_default_phase(const source_phase_t* phase) {
    phase->install_source_tree();
}

static void install_default_phase(const interface_phase_t* phase) {
    phase->install_headers_from_source();
}

static void install_default_phase(const library_phase_t* phase) {
    const auto sources = phase->install<source_phase_t>();
    std::vector<phase_base_t::built_t> source_files;

    for (const auto& source_file : filesystem::find(
        sources.root(),
        filesystem::find_include_predicate_t::is_regular,
        filesystem::find_descend_predicate_t::descend_all
    )) {
        if (is_default_library_source_file(source_file)) {
            source_files.push_back(phase->build(source_file));
        }
    }

    if (!source_files.empty()) {
        const auto library = phase->build_library(source_files, {});
        phase->install_library(library);
    }

}

static void install_default_phase(const binary_phase_t* phase) {
    const auto sources = phase->install<source_phase_t>();
    const auto cli_path = sources.root() / filesystem::relative_path_t(workspace_graph::CLI_CPP);
    if (filesystem::exists(cli_path)) {
        phase->install_binary("cli", { phase->build(filesystem::rooted_path_t(sources.root(), filesystem::relative_path_t(workspace_graph::CLI_CPP))) });
    }
}

discovered_module_dependencies_t discover_module_dependencies(
    const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module
) {
    const auto library_source_files = library_dependency_source_files(module);
    const auto library_dependencies = source_dependencies::scan_sources(
        module,
        library_source_files,
        &module,
        source_dependencies::dependency_mode_t::MODULE
    );
    const std::vector<filesystem::rooted_path_t> builder_source_files {
        filesystem::rooted_path_t(
            module.source_dir(),
            filesystem::relative_path_t(workspace_graph::BUILDER_CPP)
        )
    };
    const auto builder_dependencies = source_dependencies::scan_sources(
        module,
        builder_source_files,
        &module,
        source_dependencies::dependency_mode_t::BUILDER
    );

    return discovered_module_dependencies_t {
        .module_dependencies = source_dependencies::module_names(library_dependencies.dependencies),
        .builder_dependencies = source_dependencies::module_names(builder_dependencies.dependencies)
    };
}

struct library_scc_member_t {
    workspace_graph::module_t* module;
    std::unique_ptr<library_phase_t> phase;
    filesystem::path_t artifact_dir;
    filesystem::path_t build_dir;
    filesystem::path_t install_dir;
    filesystem::path_t started;
    filesystem::path_t complete;
};

static std::unique_ptr<library_phase_t> make_library_phase(workspace_graph::module_t& module) {
    std::unique_ptr<phase_base_t> phase;
    phase = std::make_unique<source_phase_t>(module, std::move(phase));
    phase = std::make_unique<interface_phase_t>(module, std::move(phase));
    return std::make_unique<library_phase_t>(module, std::move(phase));
}

static std::vector<library_scc_member_t> make_library_scc_members(const module_list_t& modules) {
    std::vector<library_scc_member_t> result;
    result.reserve(modules.size());

    for (auto* module : modules) {
        auto phase = make_library_phase(*module);
        const auto artifact_dir = phase_artifact_dir(*module, "library", library_phase_cache_key(*module));
        result.push_back(library_scc_member_t {
            .module = module,
            .phase = std::move(phase),
            .artifact_dir = artifact_dir,
            .build_dir = artifact_dir / filesystem::relative_path_t("build"),
            .install_dir = artifact_dir / filesystem::relative_path_t("install"),
            .started = artifact_store::started_marker(artifact_dir),
            .complete = artifact_store::completed_marker(artifact_dir)
        });
    }

    return result;
}

static bool all_library_scc_members_complete(const std::vector<library_scc_member_t>& members) {
    return std::all_of(members.begin(), members.end(), [](const auto& member) {
        return filesystem::exists(member.complete);
    });
}

static void update_latest_library_scc_members(const std::vector<library_scc_member_t>& members) {
    for (const auto& member : members) {
        update_latest_phase(*member.module, "library", member.artifact_dir);
    }
}

static void remove_library_scc_members(const std::vector<library_scc_member_t>& members) {
    for (const auto& member : members) {
        artifact_store::remove_existing_path(member.artifact_dir);
    }
}

static void prepare_library_scc_members(const std::vector<library_scc_member_t>& members) {
    for (const auto& member : members) {
        if (filesystem::exists(member.started)) {
            throw std::runtime_error(std::format(
                "m03gagbhsujjf63n0w3r2w4q6h_build_phases::prepare_library_scc_members: re-entry detected for library phase of module '{}'",
                member.module->name()
            ));
        }
    }

    remove_library_scc_members(members);

    for (const auto& member : members) {
        filesystem::create_directories(member.build_dir);
        filesystem::create_directories(member.install_dir);
        filesystem::touch(member.started);
    }
}

static module_library_root_map_t staged_library_roots(const std::vector<library_scc_member_t>& members) {
    module_library_root_map_t result;
    for (const auto& member : members) {
        result.emplace(member.module, member.install_dir);
    }
    return result;
}

static bool has_library_validation(const library_phase_t& phase, std::string_view name) {
    return std::any_of(phase.validations().begin(), phase.validations().end(), [name](const auto& validation) {
        return validation.name == name;
    });
}

static void ensure_public_api_validation(const library_phase_t& phase) {
    if (has_library_validation(phase, "public_api")) {
        return ;
    }

    const auto sources = phase.install<source_phase_t>();
    const auto public_api_test = sources.root() / filesystem::relative_path_t("test/public_api.cpp");
    if (filesystem::exists(public_api_test)) {
        phase.validate_library("public_api", { phase.source("test/public_api.cpp") });
    }
}

static void run_library_validation(
    const library_scc_member_t& member,
    const library_phase_t::validation_t& validation,
    const module_library_root_map_t& staged_roots
) {
    const auto source_file_paths = compiler_source_files(validation.source_files);
    const auto dependencies = source_dependencies::dependency_modules(
        *member.module,
        source_file_paths,
        true,
        source_dependencies::dependency_mode_t::MODULE,
        library_dependency_source_files
    );
    const auto interfaces = install_interfaces_from_modules(dependencies);
    const auto link_inputs = link_inputs_from_modules(dependencies, staged_roots);
    const auto validation_name = binary_target_relative_output_path(validation.name);
    const auto validation_build_dir = member.build_dir
        / filesystem::relative_path_t("validation")
        / validation_name;
    const auto binary = validation_build_dir / filesystem::relative_path_t("runner");

    m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::build_binary(
        validation_build_dir,
        include_dirs_from_outputs(interfaces),
        source_file_paths,
        validation.defines,
        link_inputs,
        binary
    );

    std::vector<std::string> process_args;
    process_args.reserve(validation.arguments.size() + 1);
    process_args.push_back(binary.string());
    process_args.insert(process_args.end(), validation.arguments.begin(), validation.arguments.end());

    m03gagbhsvr0m5w15urj0o291m_process::create_and_wait_checked(
        m03gagbhsvr0m5w15urj0o291m_process::command_t(process_args, validation_build_dir)
    );
}

static void run_library_scc_validations(const std::vector<library_scc_member_t>& members) {
    const auto staged_roots = staged_library_roots(members);

    for (const auto& member : members) {
        for (const auto& validation : member.phase->validations()) {
            run_library_validation(member, validation, staged_roots);
        }
    }
}

static void complete_library_scc_members(const std::vector<library_scc_member_t>& members) {
    for (const auto& member : members) {
        filesystem::touch(member.complete);
    }
    for (const auto& member : members) {
        filesystem::remove(member.started);
    }
    update_latest_library_scc_members(members);
}

template <class phase_t>
void phase_base_t::run_phase(const phase_t& requested_phase) const {
    m03gagbhsx4j5z28bqkac3dhhh_shared_library::loader_t loader(
        requested_phase.builder_plugin(),
        m03gagbhsx4j5z28bqkac3dhhh_shared_library::lifetime_t::PROCESS,
        m03gagbhsx4j5z28bqkac3dhhh_shared_library::symbol_resolution_t::LAZY,
        m03gagbhsx4j5z28bqkac3dhhh_shared_library::symbol_visibility_t::LOCAL
    );
    const auto symbol_name = std::format("phase__{}", requested_phase.name());
    using fn_t = void (*)(const phase_t*);
    if (const auto symbol = loader.resolve_optional(symbol_name.c_str())) {
        fn_t fn = *symbol;
        fn(&requested_phase);
    } else {
        install_default_phase(&requested_phase);
    }
}

filesystem::path_t phase_base_t::install_library_scc(const library_phase_t& requested_phase) const {
    module_dependency_map_t direct_dependencies_by_module;
    auto scc_modules = library_scc_modules(requested_phase.module(), direct_dependencies_by_module);
    if (!contains_module(scc_modules, requested_phase.module())) {
        throw std::logic_error(std::format(
            "m03gagbhsujjf63n0w3r2w4q6h_build_phases::phase_base_t::install_library_scc: SCC for module '{}' did not include the module itself",
            requested_phase.module().name()
        ));
    }

    auto members = make_library_scc_members(scc_modules);
    for (const auto& member : members) {
        member.phase->artifact_dir(member.artifact_dir);
    }

    if (all_library_scc_members_complete(members)) {
        update_latest_library_scc_members(members);
        for (const auto& member : members) {
            if (member.module == &requested_phase.module()) {
                return member.install_dir;
            }
        }
    }

    try {
        {
            m03gagbhsyhlx2pk5sdabbr1sx_signal_handler::scoped_termination_guard_t termination_guard;

            prepare_library_scc_members(members);

            for (const auto& member : members) {
                run_phase(*member.phase);
            }

            for (const auto& member : members) {
                ensure_public_api_validation(*member.phase);
            }

            run_library_scc_validations(members);
        }

        complete_library_scc_members(members);

        for (const auto& member : members) {
            if (member.module == &requested_phase.module()) {
                return member.install_dir;
            }
        }
    } catch (...) {
        remove_library_scc_members(members);
        throw ;
    }

    throw std::logic_error(std::format(
        "m03gagbhsujjf63n0w3r2w4q6h_build_phases::phase_base_t::install_library_scc: no installed root for module '{}'",
        requested_phase.module().name()
    ));
}

template <class phase_t>
typename phase_t::installed_t phase_base_t::install(const phase_t& requested_phase) const {

    if constexpr (std::is_same_v<phase_t, binary_phase_t>) {
        m03gagbhsyhlx2pk5sdabbr1sx_signal_handler::scoped_termination_guard_t termination_guard;

        run_phase(requested_phase);

        return typename phase_t::installed_t(m_module.artifact_latest_dir() / filesystem::relative_path_t("binary"));
    } else if constexpr (std::is_same_v<phase_t, library_phase_t>) {
        return typename phase_t::installed_t(install_library_scc(requested_phase));
    } else {
        if constexpr (std::is_same_v<phase_t, source_phase_t>) {
            requested_phase.artifact_dir(phase_artifact_dir(requested_phase.module(), requested_phase.name(), source_phase_cache_key(requested_phase.module())));
        } else if constexpr (std::is_same_v<phase_t, interface_phase_t>) {
            requested_phase.artifact_dir(phase_artifact_dir(requested_phase.module(), requested_phase.name(), interface_phase_cache_key(requested_phase.module())));
        }

        const auto artifact_dir = requested_phase.artifact_dir();
        const auto build_dir = requested_phase.build_dir();
        const auto install_dir = requested_phase.install_dir();
        const auto started = artifact_store::started_marker(artifact_dir);
        const auto complete = artifact_store::completed_marker(artifact_dir);

        if (m03gagbhsnusi43zogoacgj2ez_filesystem::exists(complete)) {
            update_latest_phase(requested_phase.module(), requested_phase.name(), artifact_dir);
            return typename phase_t::installed_t(requested_phase.install_dir());
        }

        if (m03gagbhsnusi43zogoacgj2ez_filesystem::exists(started)) {
            throw std::runtime_error(std::format("m03gagbhsujjf63n0w3r2w4q6h_build_phases::phase_base_t::install: re-entry detected for phase '{}'", requested_phase.name()));
        }

        if (artifact_store::exists_or_symlink(artifact_dir)) {
            artifact_store::remove_existing_path(artifact_dir);
        }

        try {
            {
                m03gagbhsyhlx2pk5sdabbr1sx_signal_handler::scoped_termination_guard_t termination_guard;

                if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(build_dir)) {
                    m03gagbhsnusi43zogoacgj2ez_filesystem::create_directories(build_dir);
                }
                m03gagbhsnusi43zogoacgj2ez_filesystem::touch(started);
                m03gagbhsnusi43zogoacgj2ez_filesystem::create_directories(install_dir);

                run_phase(requested_phase);
            }

            m03gagbhsnusi43zogoacgj2ez_filesystem::touch(complete);
            m03gagbhsnusi43zogoacgj2ez_filesystem::remove(started);
            update_latest_phase(requested_phase.module(), requested_phase.name(), artifact_dir);
            return typename phase_t::installed_t(requested_phase.install_dir());
        } catch (...) {
            m03gagbhsnusi43zogoacgj2ez_filesystem::remove_all(artifact_dir);
            throw ;
        }
    }
}

template <class phase_t>
typename phase_t::installed_t phase_base_t::install() const {
    for (const auto* phase = this; phase != nullptr; phase = phase->previous_phase()) {
        if (const auto* requested_phase = dynamic_cast<const phase_t*>(phase)) {
            return install(*requested_phase);
        }
    }

    throw std::runtime_error(std::format("m03gagbhsujjf63n0w3r2w4q6h_build_phases::phase_base_t::install: phase '{}' cannot install requested phase because it is not in its phase chain", name()));
}

template source_phase_t::installed_t phase_base_t::install<source_phase_t>() const;
template interface_phase_t::installed_t phase_base_t::install<interface_phase_t>() const;
template library_phase_t::installed_t phase_base_t::install<library_phase_t>() const;
template binary_phase_t::installed_t phase_base_t::install<binary_phase_t>() const;

} // namespace m03gagbhsujjf63n0w3r2w4q6h_build_phases
