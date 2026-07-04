#include "workspace_graph.h"

#include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
#include <m03gagbhsqfsqblhwvelrou7nc_json/external/json.hpp>
#include <m03gagbht2l61mj6qitacwbmea_byte_stream/byte_stream.h>

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <format>
#include <limits>
#include <stack>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace m03gagbhsp2drqq3gkop8pzfrm_workspace_graph {

static constexpr const char* WORKSPACE_ROOT_ENV = "BUILDER_WORKSPACE_ROOT";
static constexpr const char* ARTIFACT_ROOT_ENV = "BUILDER_ARTIFACT_ROOT";
static constexpr const char* BOOTSTRAP_SEED_MODULE = "m03gagbhst621faiop1rztfkqp_builder_cli";
static constexpr const char* BOOTSTRAP_SEED_WORKSPACE = "foundation";

struct json_workspace_order_manifest_t {
    std::vector<std::string> workspaces;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(json_workspace_order_manifest_t, workspaces)

struct json_module_t {
    std::vector<std::string> module_dependencies;
    std::vector<std::string> builder_dependencies;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(json_module_t, module_dependencies, builder_dependencies)

struct module_info_t {
    int index;
    int lowlink;
    bool on_stack;
};

class module_scc_t {
public:
    void add_module(module_t& module);
    void add_dependency(module_scc_t& dependency);
    const std::vector<module_t*>& modules() const;
    const std::vector<module_scc_t*>& dependencies() const;

private:
    std::vector<module_t*> m_modules;
    std::vector<module_scc_t*> m_dependencies;
};

class workspace_graph_storage_t {
public:
    void clear_sccs();
    void scc(module_t& module, module_scc_t& scc);
    module_scc_t& scc(const module_t& module) const;

private:
    std::unordered_map<const module_t*, module_scc_t*> m_scc_by_module;
};

static void path_env(const char* name, const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& path) {
    if (setenv(name, path.c_str(), 1) == -1) {
        throw std::runtime_error(std::format("m03gagbhsp2drqq3gkop8pzfrm_workspace_graph: failed to set {}: {}", name, std::strerror(errno)));
    }
}

module_name_t::module_name_t(std::string_view unique_name):
    m_unique_name(unique_name)
{
    if (unique_name.size() <= first_friendly_name_char_pos) {
        throw std::invalid_argument(std::format("m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t: module name must be at least {} characters long", first_friendly_name_char_pos));
    }

    if (unique_name[m_pos] != 'm') {
        throw std::invalid_argument(std::format("m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t: module name '{}' must contain 'm' at position {}", unique_name, m_pos));
    }

    if (unique_name[underscore_pos] != '_') {
        throw std::invalid_argument(std::format("m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t: module name '{}' must contain '_' at position {}", unique_name, underscore_pos));
    }

    const auto base36_decoded_uuidv7 = base36_uuidv7_bytes(std::string_view(unique_name).substr(base36_decoded_uuidv7_start, base36_converted_uuidv7_size));
    const auto uuidv7 = m03gagbhtft23yhjwpp881tfmc_uuid::uuid(std::span<const std::byte>(base36_decoded_uuidv7));
    const auto got_version = uuidv7.version();
    const auto expected_version = 7;
    if (got_version != expected_version) {
        throw std::invalid_argument(std::format("m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t: module name '{}' contains UUIDv7 with version {}, expected {}", unique_name, got_version, expected_version));
    }

    for (const char c : std::string_view(unique_name).substr(first_friendly_name_char_pos)) {
        if (c != '_' && !std::isalnum(static_cast<unsigned char>(c))) {
            throw std::invalid_argument(std::format("m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t: module name '{}' contains invalid character '{}'", unique_name, c));
        }
    }
}

module_name_t module_name_t::from_friendly_name(std::string_view friendly_name) {
    if (friendly_name.empty()) {
        throw std::invalid_argument("m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t::from_friendly_name: friendly name must not be empty");
    }

    for (const char c : friendly_name) {
        if (c != '_' && !std::isalnum(static_cast<unsigned char>(c))) {
            throw std::invalid_argument(std::format("m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t::from_friendly_name: friendly name '{}' contains invalid character '{}'", friendly_name, c));
        }
    }

    const auto uuidv7 = m03gagbhtft23yhjwpp881tfmc_uuid::uuid::generate(7);
    auto base36_encoded_uuidv7 = m03gagbht2l61mj6qitacwbmea_byte_stream::byte_stream_t(uuidv7.bytes()).to_radix(36);
    if (base36_encoded_uuidv7.size() < base36_converted_uuidv7_size) {
        base36_encoded_uuidv7 = std::string(base36_converted_uuidv7_size - base36_encoded_uuidv7.size(), '0') + base36_encoded_uuidv7;
    }
    if (base36_encoded_uuidv7.size() != base36_converted_uuidv7_size) {
        throw std::logic_error(std::format("m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t::from_friendly_name: base36-encoded UUIDv7 '{}' is not {} characters long", base36_encoded_uuidv7, base36_converted_uuidv7_size));
    }

    const auto validated_name = module_name_t::validated_name_t{std::format("m{}_{}", base36_encoded_uuidv7, friendly_name)};
    return module_name_t(validated_name);
}

const std::string& module_name_t::unique_name() const {
    return m_unique_name;
}

std::string module_name_t::friendly_name() const {
    return m_unique_name.substr(first_friendly_name_char_pos);
}

m03gagbhtft23yhjwpp881tfmc_uuid::uuid module_name_t::uuid() const {
    const auto base36_decoded_uuidv7 = base36_uuidv7_bytes(std::string_view(m_unique_name).substr(base36_decoded_uuidv7_start, base36_converted_uuidv7_size));
    return m03gagbhtft23yhjwpp881tfmc_uuid::uuid(std::span<const std::byte>(base36_decoded_uuidv7));
}

bool module_name_t::operator==(const module_name_t& other) const {
    return m_unique_name == other.m_unique_name;
}

module_name_t::module_name_t(module_name_t::validated_name_t validated_name) noexcept:
    m_unique_name(std::move(validated_name.name))
{
}

std::array<std::byte, 16> module_name_t::base36_uuidv7_bytes(std::string_view view) const {
    const auto base36_decoded_uuidv7 = m03gagbht2l61mj6qitacwbmea_byte_stream::byte_stream_t::from_radix(view, 36);

    std::array<std::byte, 16> result{};

    if (result.size() < base36_decoded_uuidv7.size()) {
        throw std::invalid_argument(std::format("m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t::base36_uuidv7_bytes: base36-decoded UUIDv7 '{}' is wider than {} bytes", base36_decoded_uuidv7, result.size()));
    }

    std::copy(
        base36_decoded_uuidv7.bytes().begin(),
        base36_decoded_uuidv7.bytes().end(),
        result.begin() + static_cast<std::ptrdiff_t>(result.size() - base36_decoded_uuidv7.size())
    );

    return result;
}

static m03gagbhsnusi43zogoacgj2ez_filesystem::path_t workspace_root() {
    const char* workspace_root_env = std::getenv(WORKSPACE_ROOT_ENV);
    if (workspace_root_env == nullptr) {
        return m03gagbhsnusi43zogoacgj2ez_filesystem::current_path();
    }

    if (*workspace_root_env == '\0') {
        throw std::runtime_error(std::format("m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::invocation_context: {} must not be empty", WORKSPACE_ROOT_ENV));
    }

    return m03gagbhsnusi43zogoacgj2ez_filesystem::path_t(workspace_root_env);
}

static m03gagbhsnusi43zogoacgj2ez_filesystem::path_t artifact_root(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& workspace_root) {
    const char* artifact_root_env = std::getenv(ARTIFACT_ROOT_ENV);
    if (artifact_root_env == nullptr) {
        return workspace_root / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("artifacts");
    }

    if (*artifact_root_env == '\0') {
        throw std::runtime_error(std::format("m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::invocation_context: {} must not be empty", ARTIFACT_ROOT_ENV));
    }

    return m03gagbhsnusi43zogoacgj2ez_filesystem::path_t(artifact_root_env);
}

invocation_context_t invocation_context() {
    const auto root = workspace_root();
    const auto context = invocation_context_t {
        .workspace_root = root,
        .artifact_root = artifact_root(root)
    };

    path_env(WORKSPACE_ROOT_ENV, context.workspace_root);
    path_env(ARTIFACT_ROOT_ENV, context.artifact_root);

    return context;
}

workspace_graph_t::workspace_graph_t(m03gagbhsnusi43zogoacgj2ez_filesystem::path_t workspace_root, m03gagbhsnusi43zogoacgj2ez_filesystem::path_t artifact_dir):
    m_root(std::move(workspace_root)),
    m_artifact_root(std::move(artifact_dir)),
    m_bootstrap_seed_workspace(nullptr),
    m_bootstrap_seed_module(nullptr),
    m_storage(new workspace_graph_storage_t)
{
    json_workspace_order_manifest_t json_workspace_order_manifest;
    const auto workspaces_json_file = root() / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(WORKSPACES_JSON);
    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(workspaces_json_file)) {
        throw std::runtime_error(std::format("m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t::load_module_index: file does not exist: '{}'", workspaces_json_file));
    }

    std::ifstream ifs(workspaces_json_file.string());
    if (!ifs) {
        throw std::runtime_error(std::format("m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t::load_module_index: failed to open file '{}'", workspaces_json_file));
    }

    try {
        nlohmann::json json = nlohmann::json::parse(ifs);
        json_workspace_order_manifest = json.get<decltype(json_workspace_order_manifest)>();
    } catch (const nlohmann::json::parse_error& e) {
        throw std::runtime_error(std::format("m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t::load_module_index: failed to parse JSON file '{}': {}", workspaces_json_file, e.what()));
    } catch (const nlohmann::json::exception& e) {
        throw std::runtime_error(std::format("m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t::load_module_index: failed to get JSON workspace order manifest from file '{}': {}", workspaces_json_file, e.what()));
    }

    for (std::size_t i = 0; i < json_workspace_order_manifest.workspaces.size(); ++i) {
        const auto workspace_relative_path = m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(json_workspace_order_manifest.workspaces[i]);
        if (m_workspace_by_relative_path.find(workspace_relative_path) != m_workspace_by_relative_path.end()) {
            throw std::runtime_error(std::format("m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t::load_module_index: duplicate workspace '{}' in '{}'", workspace_relative_path, workspaces_json_file));
        }

        auto workspace = new workspace_t(*this, workspace_relative_path, static_cast<uint32_t>(i));

        m_workspace_by_relative_path.emplace(workspace_relative_path, workspace);
    }

    for (const auto* workspace : workspaces()) {
        const auto workspace_dir = root() / workspace->relative_path();
        if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(workspace_dir)) {
            continue ;
        }

        for (const auto& module_dir : m03gagbhsnusi43zogoacgj2ez_filesystem::find(workspace_dir, m03gagbhsnusi43zogoacgj2ez_filesystem::find_include_predicate_t::is_dir, m03gagbhsnusi43zogoacgj2ez_filesystem::find_descend_predicate_t::descend_none)) {
            const auto module_name = module_name_t(module_dir.relative_path().string());
            const auto module_json_path = module_dir.path() / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(MODULE_JSON);
            if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(module_json_path)) {
                continue ;
            }

            const auto [it, inserted] = m_workspace_by_module_name.emplace(module_name, workspace);
            if (!inserted) {
                throw std::runtime_error(std::format(
                    "m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t::load_module_index: duplicate module name '{}' found in workspaces '{}' and '{}'; module names are globally unique",
                    module_name,
                    it->second->relative_path(),
                    workspace->relative_path()
                ));
            }
        }
    }
}

const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& workspace_graph_t::root() const {
    return m_root;
}

const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& workspace_graph_t::artifact_root() const {
    return m_artifact_root;
}

module_t& workspace_graph_t::bootstrap_seed_module() const {
    if (m_bootstrap_seed_module == nullptr) {
        throw std::runtime_error("m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t::bootstrap_seed_module: bootstrap seed module has not been discovered");
    }

    return *m_bootstrap_seed_module;
}

bool workspace_graph_t::is_active_builder_bootstrap_module(const module_t& module) const {
    if (m_bootstrap_seed_workspace == nullptr) {
        throw std::runtime_error("m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t::is_active_builder_bootstrap_module: bootstrap seed workspace has not been discovered");
    }
    if (m_bootstrap_seed_module == nullptr) {
        throw std::runtime_error("m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t::is_active_builder_bootstrap_module: bootstrap seed module has not been discovered");
    }
    if (&module.workspace() != m_bootstrap_seed_workspace) {
        return false;
    }

    std::vector<const module_t*> pending { m_bootstrap_seed_module };
    std::unordered_set<const module_t*> visited;

    while (!pending.empty()) {
        const auto* current = pending.back();
        pending.pop_back();

        if (!visited.insert(current).second) {
            continue ;
        }

        if (current == &module) {
            return true;
        }

        for (const auto* dependency : current->dependencies()) {
            pending.push_back(dependency);
        }
        for (const auto* dependency : current->builder_dependencies()) {
            pending.push_back(dependency);
        }
    }

    return false;
}

workspace_t::workspace_t(workspace_graph_t& workspace_graph, m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t relative_path, uint32_t order_position):
    m_workspace_graph(&workspace_graph),
    m_relative_path(std::move(relative_path)),
    m_order_position(order_position)
{
}

workspace_graph_t& workspace_t::graph() const {
    return *m_workspace_graph;
}

const m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t& workspace_t::relative_path() const {
    return m_relative_path;
}

uint32_t workspace_t::order_position() const {
    return m_order_position;
}

module_t::module_t(workspace_t& workspace, module_name_t name, version_t version):
    m_workspace(&workspace),
    m_version(version),
    m_name(std::move(name))
{
}

workspace_t& module_t::workspace() const {
    return *m_workspace;
}

version_t module_t::version() const {
    return m_version;
}

void module_t::version(version_t version) {
    m_version = version;
}

void module_t::add_dependency(module_t& dependency) {
    m_dependencies.insert(&dependency);
}

void module_t::add_builder_dependency(module_t& dependency) {
    m_builder_dependencies.insert(&dependency);
}

void module_scc_t::add_module(module_t& module) {
    m_modules.push_back(&module);
}

void module_scc_t::add_dependency(module_scc_t& dependency) {
    m_dependencies.push_back(&dependency);
}

const std::vector<module_t*>& module_scc_t::modules() const {
    return m_modules;
}

const std::vector<module_scc_t*>& module_scc_t::dependencies() const {
    return m_dependencies;
}

void workspace_graph_storage_t::clear_sccs() {
    m_scc_by_module.clear();
}

void workspace_graph_storage_t::scc(module_t& module, module_scc_t& scc) {
    m_scc_by_module[&module] = &scc;
}

module_scc_t& workspace_graph_storage_t::scc(const module_t& module) const {
    const auto it = m_scc_by_module.find(&module);
    if (it == m_scc_by_module.end()) {
        throw std::runtime_error(std::format("m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_storage_t::scc: module '{}' has no SCC", module.name()));
    }

    return *it->second;
}

module_t::groups_t module_t::closure_groups() const {
    return workspace().graph().closure_groups(*this);
}

module_t::groups_t workspace_graph_t::closure_groups(const module_t& module) const {
    std::unordered_set<const module_scc_t*> visited_sccs;
    module_t::groups_t result;

    const auto collect_scc = [&]<class self_t>(self_t& self, const module_scc_t& current_scc) -> void {
        if (!visited_sccs.insert(&current_scc).second) {
            return ;
        }

        for (const auto* dependency : current_scc.dependencies()) {
            self(self, *dependency);
        }

        module_t::group_t group;
        group.reserve(current_scc.modules().size());
        for (auto* module : current_scc.modules()) {
            group.push_back(module);
        }
        result.push_back(std::move(group));
    };

    collect_scc(collect_scc, m_storage->scc(module));

    return result;
}

static std::filesystem::file_time_type latest_write_time(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& directory) {
    auto latest_module_file = m03gagbhsnusi43zogoacgj2ez_filesystem::last_write_time(directory);

    for (const auto& path : m03gagbhsnusi43zogoacgj2ez_filesystem::find(directory, m03gagbhsnusi43zogoacgj2ez_filesystem::find_include_predicate_t::include_all, m03gagbhsnusi43zogoacgj2ez_filesystem::find_descend_predicate_t::descend_all)) {
        latest_module_file = std::max(latest_module_file, m03gagbhsnusi43zogoacgj2ez_filesystem::last_write_time(path.path()));
    }

    return latest_module_file;
}

version_t::version_t(uint64_t value):
    value(value)
{
}

version_t::version_t(const std::filesystem::file_time_type& file_time_type):
    version_t(static_cast<uint64_t>(file_time_type.time_since_epoch().count() - std::numeric_limits<std::filesystem::file_time_type::duration::rep>::min()))
{
}

version_t::version_t(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& directory):
    version_t(latest_write_time(directory))
{
}

const module_name_t& module_t::name() const {
    return m_name;
}

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t module_t::source_dir() const {
    return m_workspace->graph().root() / m_workspace->relative_path() / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(m_name.unique_name());
}

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t module_t::artifact_base_dir() const {
    return m_workspace->graph().artifact_root() / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(m_name.unique_name());
}

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t module_t::artifact_dir() const {
    const auto versioned_dir_name = std::format("{}@{}", m_name, version().value);
    return artifact_base_dir() / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(versioned_dir_name);
}

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t module_t::artifact_latest_dir() const {
    return artifact_base_dir() / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("latest");
}

module_t* workspace_t::find_module(const module_name_t& module_name) const {
    auto it = m_module_by_name.find(module_name);
    if (it == m_module_by_name.end()) {
        return nullptr;
    }

    return it->second;
}

void workspace_t::add_module(module_t* module) {
    m_module_by_name.emplace(module->name(), module);
}

static bool module_less(const module_t* lhs, const module_t* rhs) {
    if (lhs->workspace().order_position() != rhs->workspace().order_position()) {
        return lhs->workspace().order_position() < rhs->workspace().order_position();
    }

    return lhs->name().unique_name() < rhs->name().unique_name();
}

std::vector<module_t*> workspace_t::modules() const {
    std::vector<module_t*> result;
    result.reserve(m_module_by_name.size());

    for (const auto& [_, module] : m_module_by_name) {
        result.push_back(module);
    }

    std::sort(result.begin(), result.end(), module_less);

    return result;
}

std::vector<module_t*> module_t::dependencies() {
    std::vector<module_t*> result;
    result.reserve(m_dependencies.size());

    for (auto* dependency : m_dependencies) {
        result.push_back(dependency);
    }

    std::sort(result.begin(), result.end(), module_less);

    return result;
}

std::vector<const module_t*> module_t::dependencies() const {
    std::vector<const module_t*> result;
    result.reserve(m_dependencies.size());

    for (const auto* dependency : m_dependencies) {
        result.push_back(dependency);
    }

    std::sort(result.begin(), result.end(), module_less);

    return result;
}

std::vector<module_t*> module_t::builder_dependencies() {
    std::vector<module_t*> result;
    result.reserve(m_builder_dependencies.size());

    for (auto* dependency : m_builder_dependencies) {
        result.push_back(dependency);
    }

    std::sort(result.begin(), result.end(), module_less);

    return result;
}

std::vector<const module_t*> module_t::builder_dependencies() const {
    std::vector<const module_t*> result;
    result.reserve(m_builder_dependencies.size());

    for (const auto* dependency : m_builder_dependencies) {
        result.push_back(dependency);
    }

    std::sort(result.begin(), result.end(), module_less);

    return result;
}

std::vector<const workspace_t*> workspace_graph_t::workspaces() const {
    std::vector<const workspace_t*> result;
    result.reserve(m_workspace_by_relative_path.size());

    for (const auto& [_, workspace] : m_workspace_by_relative_path) {
        result.push_back(workspace);
    }

    std::sort(result.begin(), result.end(), [](const auto* lhs, const auto* rhs) {
        if (lhs->order_position() != rhs->order_position()) {
            return lhs->order_position() < rhs->order_position();
        }

        return lhs->relative_path().string() < rhs->relative_path().string();
    });

    return result;
}

std::vector<const module_t*> workspace_graph_t::modules() const {
    std::vector<const module_t*> result;

    for (const auto* workspace : workspaces()) {
        for (const auto* module : workspace->modules()) {
            result.push_back(module);
        }
    }

    std::sort(result.begin(), result.end(), module_less);

    return result;
}

std::vector<module_name_t> workspace_graph_t::module_names() const {
    std::vector<module_name_t> result;
    
    result.reserve(m_workspace_by_module_name.size());

    for (const auto& [module_name, _] : m_workspace_by_module_name) {
        result.push_back(module_name);
    }

    std::sort(result.begin(), result.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.unique_name() < rhs.unique_name();
    });

    return result;
}

module_t* workspace_graph_t::discover_module_impl(module_name_t module_name) {
    const auto it = m_workspace_by_module_name.find(module_name);
    if (it == m_workspace_by_module_name.end()) {
        throw std::runtime_error(std::format(
            "m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t::discover_module_impl: module '{}' not found in workspace graph",
            module_name
        ));
    }

    const auto workspace_relative_path = it->second->relative_path();
    auto workspace_it = m_workspace_by_relative_path.find(workspace_relative_path);
    if (workspace_it == m_workspace_by_relative_path.end()) {
        throw std::runtime_error(std::format("m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::discover_module_impl: workspace '{}' is not listed in workspace order manifest '{}'", workspace_relative_path, WORKSPACES_JSON));
    }
    auto workspace = workspace_it->second;
    if (auto* discovered_module = workspace->find_module(module_name); discovered_module != nullptr) {
        return discovered_module;
    }

    const auto module_directory = root() / workspace_relative_path / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(module_name.unique_name());
    const auto module_version = version_t(module_directory);
    auto module = new module_t(*workspace, module_name, module_version);

    workspace->add_module(module);

    json_module_t json_module;
    {
        const auto module_json_path = module_directory / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(MODULE_JSON);
        if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(module_json_path)) {
            throw std::runtime_error(std::format("m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::discover_module_impl: file does not exist: '{}'", module_json_path));
        }

        std::ifstream ifs(module_json_path.string());
        if (!ifs) {
            throw std::runtime_error(std::format("m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::discover_module_impl: failed to open file '{}'", module_json_path));
        }

        try {
            nlohmann::json json = nlohmann::json::parse(ifs);
            json_module = json.get<decltype(json_module)>();
        } catch (const nlohmann::json::parse_error& e) {
            throw std::runtime_error(std::format("m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::discover_module_impl: failed to parse JSON file '{}': {}", module_json_path, e.what()));
        } catch (const nlohmann::json::exception& e) {
            throw std::runtime_error(std::format("m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::discover_module_impl: failed to get JSON module from file '{}': {}", module_json_path, e.what()));
        }
    }

    for (const auto& module_dependency : json_module.module_dependencies) {
        module->add_dependency(*discover_module_impl(module_name_t(module_dependency)));
    }

    for (const auto& builder_dependency : json_module.builder_dependencies) {
        module->add_builder_dependency(*discover_module_impl(module_name_t(builder_dependency)));
    }

    return module;
}

static void strong_connect(
    module_t* module,
    uint32_t& index,
    std::stack<module_t*>& S,
    std::unordered_map<module_t*, module_info_t>& module_info_by_module,
    workspace_graph_storage_t& graph_storage
) {
    const auto module_info_by_module_it = module_info_by_module.find(module);
    if (module_info_by_module_it == module_info_by_module.end()) {
        throw std::runtime_error("m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::strong_connect: module not found in module_info_by_module");
    }

    auto& module_info = module_info_by_module_it->second;
    module_info.index = index;
    module_info.lowlink = index;
    ++index;
    S.push(module);
    module_info.on_stack = true;

    for (auto* dependency : module->dependencies()) {
        const auto dependency_module_info_by_module_it = module_info_by_module.find(dependency);
        if (dependency_module_info_by_module_it == module_info_by_module.end()) {
            throw std::runtime_error("m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::strong_connect: dependency module not found in module_info_by_module");
        }
        const auto& dependency_module_info = dependency_module_info_by_module_it->second;
        if (dependency_module_info.index == -1) {
            strong_connect(dependency, index, S, module_info_by_module, graph_storage);
            module_info.lowlink = std::min(module_info.lowlink, dependency_module_info.lowlink);
        } else if (dependency_module_info.on_stack) {
            module_info.lowlink = std::min(module_info.lowlink, dependency_module_info.index);
        }
    }

    if (module_info.lowlink == module_info.index) {
        module_scc_t* module_scc = new module_scc_t;
        while (1) {
            const auto neighbor_module = S.top();
            S.pop();
            const auto neighbor_module_info_by_module_it = module_info_by_module.find(neighbor_module);
            if (neighbor_module_info_by_module_it == module_info_by_module.end()) {
                throw std::runtime_error("m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::strong_connect: neighbor module not found in module_info_by_module");
            }
            auto& neighbor_module_info = neighbor_module_info_by_module_it->second;
            neighbor_module_info.on_stack = false;
            module_scc->add_module(*neighbor_module);
            graph_storage.scc(*neighbor_module, *module_scc);
            
            if (neighbor_module == module) {
                break ;
            }
        }
    }
}

static version_t version_sccs(module_scc_t* scc, std::unordered_map<module_scc_t*, version_t>& visited, version_t minimum_version) {
    if (auto it = visited.find(scc); it != visited.end()) {
        return it->second;
    }

    version_t result = minimum_version;

    for (const auto& dependency : scc->dependencies()) {
        result.value = std::max(result.value, version_sccs(dependency, visited, minimum_version).value);
    }

    for (const auto& module : scc->modules()) {
        result.value = std::max(result.value, module->version().value);
    }

    for (auto& module : scc->modules()) {
        module->version(result);
    }

    visited.emplace(scc, result);

    return result;
}

static void validate_module(
    const workspace_graph_t& workspace_graph,
    module_t* module,
    std::unordered_set<module_t*>& validated_modules
) {
    if (!validated_modules.insert(module).second) {
        return ;
    }

    const auto order_position = module->workspace().order_position();
    for (auto* module_dependency : module->dependencies()) {
        const auto module_dependency_order_position = module_dependency->workspace().order_position();
        if (!(module_dependency_order_position <= order_position)) {
            throw std::runtime_error(std::format("m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::validate_module: module (workspace: {}, module: {}, order position: {}) cannot depend on later workspace module (workspace: {}, module: {}, order position: {})", module->workspace().relative_path(), module->name(), order_position, module_dependency->workspace().relative_path(), module_dependency->name(), module_dependency_order_position));
        }

        validate_module(workspace_graph, module_dependency, validated_modules);
    }

    for (auto* builder_dependency : module->builder_dependencies()) {
        const auto builder_dependency_order_position = builder_dependency->workspace().order_position();
        const bool bootstrap_group_dependency =
            workspace_graph.is_active_builder_bootstrap_module(*module)
            && workspace_graph.is_active_builder_bootstrap_module(*builder_dependency);
        if (!(builder_dependency_order_position < order_position) && !bootstrap_group_dependency) {
            throw std::runtime_error(std::format("m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::validate_module: builder (workspace: {}, module: {}, order position: {}) cannot depend on same or later workspace module (workspace: {}, module: {}, order position: {})", module->workspace().relative_path(), module->name(), order_position, builder_dependency->workspace().relative_path(), builder_dependency->name(), builder_dependency_order_position));
        }

        validate_module(workspace_graph, builder_dependency, validated_modules);
    }
}

module_t* workspace_graph_t::discover_module(module_name_t module_name) {
    auto* result = discover_module_impl(module_name);

    auto bootstrap_seed_workspace_it = m_workspace_by_relative_path.find(m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(BOOTSTRAP_SEED_WORKSPACE));
    if (bootstrap_seed_workspace_it == m_workspace_by_relative_path.end()) {
        throw std::runtime_error(std::format("m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::discover_module: bootstrap seed workspace '{}' not found in workspace graph", BOOTSTRAP_SEED_WORKSPACE));
    }
    auto* bootstrap_seed_module = discover_module_impl(module_name_t(BOOTSTRAP_SEED_MODULE));
    m_bootstrap_seed_workspace = bootstrap_seed_workspace_it->second;
    m_bootstrap_seed_module = bootstrap_seed_module;

    std::unordered_map<module_t*, module_info_t> module_info_by_module;
    for (const auto& [_, workspace] : m_workspace_by_relative_path) {
        for (auto* module : workspace->modules()) {
            module_info_by_module[module] = module_info_t {
                .index = -1,
                .lowlink = -1,
                .on_stack = false
            };
        }
    }

    uint32_t index = 0;
    std::stack<module_t*> S;
    m_storage->clear_sccs();
    for (const auto& [module, module_info] : module_info_by_module) {
        if (module_info.index == -1) {
            strong_connect(module, index, S, module_info_by_module, *m_storage);
        }
    }

    std::unordered_map<module_scc_t*, std::unordered_set<module_scc_t*>> module_scc_dependencies_by_module_scc;
    for (const auto& [_, workspace] : m_workspace_by_relative_path) {
        for (auto* module : workspace->modules()) {
            for (auto* dependency : module->dependencies()) {
                auto& dependency_scc = m_storage->scc(*dependency);
                auto& module_scc = m_storage->scc(*module);

                if (&dependency_scc != &module_scc && module_scc_dependencies_by_module_scc[&module_scc].insert(&dependency_scc).second) {
                    module_scc.add_dependency(dependency_scc);
                }
            }
        }
    }

    std::vector<module_t*> modules;
    for (const auto& [_, workspace] : m_workspace_by_relative_path) {
        for (auto* module : workspace->modules()) {
            modules.push_back(module);
        }
    }

    const auto propagate_module_dependency_versions = [&]() {
        std::unordered_map<module_scc_t*, version_t> visited;
        for (auto* module : modules) {
            version_sccs(&m_storage->scc(*module), visited, m_bootstrap_seed_module->version());
        }
    };

    for (std::size_t i = 0; i <= modules.size(); ++i) {
        propagate_module_dependency_versions();

        bool changed = false;
        for (auto* module : modules) {
            version_t builder_version = module->version();
            for (auto* builder_dependency : module->builder_dependencies()) {
                builder_version.value = std::max(builder_version.value, builder_dependency->version().value);
            }

            if (module->version().value < builder_version.value) {
                module->version(builder_version);
                changed = true;
            }
        }

        if (!changed) {
            break ;
        }
    }

    std::unordered_set<module_t*> validated_modules;
    validate_module(*this, result, validated_modules);

    return result;
}

} // namespace m03gagbhsp2drqq3gkop8pzfrm_workspace_graph
