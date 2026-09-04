#include "workspace_graph.h"

#include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
#include <m03gagbht2l61mj6qitacwbmea_byte_stream/byte_stream.h>

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <format>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <set>
#include <iostream>
#include <optional>
#include <cassert>
#include <type_traits>

namespace m03gagbhsp2drqq3gkop8pzfrm_workspace_graph {

static constexpr const char* WORKSPACE_ROOT_ENV = "BUILDER_WORKSPACE_ROOT";
static constexpr const char* ARTIFACT_ROOT_ENV = "BUILDER_ARTIFACT_ROOT";
static void path_env(const char* name, const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& path) {
    if (setenv(name, path.c_str(), 1) == -1) {
        throw std::runtime_error(std::format("m03gagbhsp2drqq3gkop8pzfrm_workspace_graph: failed to set {}: {}", name, std::strerror(errno)));
    }
}

module_name_t::module_name_t(std::string_view unique_name):
    m_unique_name(unique_name)
{
    if (unique_name.size() <= first_friendly_name_char_pos) {
        throw std::invalid_argument(std::format("m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t: module name must be at least {} characters long", first_friendly_name_char_pos + 1));
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

bool module_name_t::operator<(const module_name_t& other) const {
    return m_unique_name < other.m_unique_name;
}

bool module_name_t::operator<=(const module_name_t& other) const {
    return m_unique_name <= other.m_unique_name;
}

module_name_t::module_name_t(module_name_t::validated_name_t validated_name) noexcept:
    m_unique_name(std::move(validated_name.name))
{
}

std::array<std::byte, 16> module_name_t::base36_uuidv7_bytes(std::string_view view) const {
    const auto base36_decoded_uuidv7 = m03gagbht2l61mj6qitacwbmea_byte_stream::byte_stream_t::from_radix(view, 36);

    std::array<std::byte, 16> result{};

    const auto& base36_decoded_uuidv7_bytes = base36_decoded_uuidv7.bytes();

    if (result.size() < base36_decoded_uuidv7_bytes.size()) {
        throw std::invalid_argument(std::format("m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t::base36_uuidv7_bytes: base36-decoded UUIDv7 '{}' is wider than {} bytes", base36_decoded_uuidv7, result.size()));
    }

    std::copy(
        base36_decoded_uuidv7_bytes.begin(),
        base36_decoded_uuidv7_bytes.end(),
        result.begin() + static_cast<std::ptrdiff_t>(result.size() - base36_decoded_uuidv7_bytes.size())
    );

    return result;
}

workspace_name_t::workspace_name_t(std::string_view name):
    m_relative_path(name),
    m_order_position(0)
{
    if (!name.starts_with("ws")) {
        throw std::runtime_error(std::format("m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_name_t: workspace name '{}' must start with 'ws'", name));
    }

    const auto order = std::string_view(name).substr(2);
    if (order.empty()) {
        throw std::invalid_argument(std::format("m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_name_t: workspace name '{}' must have a numeric suffix", name));
    }

    for (const char c : order) {
        if (c < '0' || '9' < c) {
            throw std::invalid_argument(std::format("m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_name_t: workspace name '{}' contains non-digit suffix character '{}'", name, c));
        }

        if ((std::numeric_limits<uint32_t>::max() - static_cast<uint32_t>(c - '0')) / 10 < m_order_position) {
            throw std::invalid_argument(std::format("m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_name_t: workspace name '{}' numeric suffix is too large to fit in uint32_t", name));
        }
        m_order_position = m_order_position * 10 + static_cast<uint32_t>(c - '0');
    }
}

const m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t& workspace_name_t::relative_path() const {
    return m_relative_path;
}

uint32_t workspace_name_t::order_position() const {
    return m_order_position;
}

bool workspace_name_t::operator==(const workspace_name_t& other) const {
    return m_relative_path == other.m_relative_path;
}

bool workspace_name_t::operator<(const workspace_name_t& other) const {
    return m_order_position < other.m_order_position;
}

bool workspace_name_t::operator<=(const workspace_name_t& other) const {
    return m_order_position <= other.m_order_position;
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
    m_artifact_root(std::move(artifact_dir))
{
    for (const auto& workspace_dir : m03gagbhsnusi43zogoacgj2ez_filesystem::find(root(), m03gagbhsnusi43zogoacgj2ez_filesystem::find_include_predicate_t::is_dir, m03gagbhsnusi43zogoacgj2ez_filesystem::find_descend_predicate_t::descend_none)) {
        std::optional<workspace_name_t> maybe_workspace_name;
        try {
            maybe_workspace_name = workspace_name_t(workspace_dir.relative_path().string());
        } catch (const std::invalid_argument& e) {
            std::cerr << std::format("m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t: skipping directory '{}' because it is not a valid workspace name: {}", workspace_dir, e.what()) << std::endl;
            continue ;
        } catch (...) {
            continue ;
        }

        assert(maybe_workspace_name.has_value());
        if (!maybe_workspace_name) {
            throw std::logic_error(std::format("m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t: workspace name is empty after validation for directory '{}'", workspace_dir));
        }

        auto workspace = std::make_unique<workspace_t>(*this, *maybe_workspace_name);
        if (m_workspace_by_workspace_name.emplace(*maybe_workspace_name, std::move(workspace)).second == false) {
            throw std::runtime_error(std::format("m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t: duplicate workspace name '{}'", *maybe_workspace_name));
        }
    }

    for (const auto& [workspace_name, workspace] : m_workspace_by_workspace_name) {
        const auto workspace_dir = root() / workspace_name.relative_path();
        for (const auto& module_dir : m03gagbhsnusi43zogoacgj2ez_filesystem::find(workspace_dir, m03gagbhsnusi43zogoacgj2ez_filesystem::find_include_predicate_t::is_dir, m03gagbhsnusi43zogoacgj2ez_filesystem::find_descend_predicate_t::descend_none)) {
            const auto module_name = module_name_t(module_dir.relative_path().string());
            const auto [it, inserted] = m_workspace_by_module_name.emplace(module_name, workspace.get());
            if (!inserted) {
                throw std::runtime_error(std::format(
                    "m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t::load_module_index: duplicate module name '{}' found in workspaces '{}' and '{}'; module names must be globally unique",
                    module_name,
                    it->second->name(),
                    workspace_name
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

workspace_t::workspace_t(workspace_graph_t& workspace_graph, workspace_name_t name):
    m_workspace_graph(&workspace_graph),
    m_name(std::move(name))
{
}

bool workspace_t::operator==(const workspace_t& other) const {
    return m_name == other.m_name;
}

bool workspace_t::operator<(const workspace_t& other) const {
    return m_name < other.m_name;
}

bool workspace_t::operator<=(const workspace_t& other) const {
    return m_name <= other.m_name;
}

workspace_graph_t& workspace_t::graph() const {
    return *m_workspace_graph;
}

const workspace_name_t& workspace_t::name() const {
    return m_name;
}

module_t::module_t(const workspace_t* workspace, module_name_t name, version_t version):
    m_workspace(workspace),
    m_version(version),
    m_name(std::move(name))
{
    if (m_workspace == nullptr) {
        throw std::invalid_argument("m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t::module_t: workspace must not be null");
    }
}

const workspace_t& module_t::workspace() const {
    return *m_workspace;
}

version_t module_t::version() const {
    return m_version;
}

void module_t::version(version_t version) {
    m_version = version;
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
    version_t([&] {
        using rep_t = std::filesystem::file_time_type::duration::rep;
        using unsigned_rep_t = std::make_unsigned_t<rep_t>;
        static_assert(std::numeric_limits<unsigned_rep_t>::digits <= std::numeric_limits<uint64_t>::digits);

        const auto count = static_cast<unsigned_rep_t>(file_time_type.time_since_epoch().count());
        const auto minimum = static_cast<unsigned_rep_t>(std::numeric_limits<rep_t>::min());
        return static_cast<uint64_t>(count - minimum);
    }())
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
    return m_workspace->graph().root() / m_workspace->name().relative_path() / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(m_name.unique_name());
}

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t module_t::artifact_base_dir() const {
    return m_workspace->graph().artifact_root() / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(m_name.unique_name());
}

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t module_t::artifact_latest_dir() const {
    return artifact_base_dir() / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("latest");
}

module_t* workspace_t::find_module(const module_name_t& module_name) const {
    auto it = m_module_by_name.find(module_name);
    if (it == m_module_by_name.end()) {
        return nullptr;
    }

    return it->second.get();
}

module_t* workspace_t::add_module(std::unique_ptr<module_t> module) {
    if (module == nullptr) {
        throw std::invalid_argument("m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_t::add_module: module must not be null");
    }
    if (&module->workspace() != this) {
        throw std::invalid_argument("m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_t::add_module: module belongs to another workspace");
    }
    if (m_module_by_name.contains(module->name())) {
        throw std::invalid_argument(std::format("m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_t::add_module: module '{}' already exists", module->name()));
    }

    auto* result = module.get();
    m_module_by_name.emplace(module->name(), std::move(module));
    return result;
}

static bool module_less(const module_t* lhs, const module_t* rhs) {
    if (lhs->workspace().name() != rhs->workspace().name()) {
        return lhs->workspace().name() < rhs->workspace().name();
    }

    return lhs->name().unique_name() < rhs->name().unique_name();
}

std::vector<module_t*> workspace_t::modules() const {
    std::vector<module_t*> result;
    result.reserve(m_module_by_name.size());

    for (const auto& [_, module] : m_module_by_name) {
        result.push_back(module.get());
    }

    std::sort(result.begin(), result.end(), module_less);

    return result;
}

std::vector<const workspace_t*> workspace_graph_t::workspaces() const {
    std::vector<const workspace_t*> result;
    result.reserve(m_workspace_by_workspace_name.size());

    for (const auto& [_, workspace] : m_workspace_by_workspace_name) {
        result.push_back(workspace.get());
    }

    std::sort(result.begin(), result.end(), [](const auto* lhs, const auto* rhs) {
        return lhs->name() < rhs->name();
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

std::set<module_name_t> workspace_graph_t::module_names() const {
    std::set<module_name_t> result;
    
    for (const auto& [module_name, _] : m_workspace_by_module_name) {
        result.insert(module_name);
    }

    return result;
}

module_t* workspace_graph_t::discover_module_impl(module_name_t module_name) {
    const auto it = m_workspace_by_module_name.find(module_name);
    if (it == m_workspace_by_module_name.end()) {
        throw std::runtime_error(std::format("m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t::discover_module_impl: module '{}' not found in workspace graph", module_name));
    }

    auto workspace = it->second;
    if (auto* discovered_module = workspace->find_module(module_name); discovered_module != nullptr) {
        return discovered_module;
    }

    const auto module_directory = root() / workspace->name().relative_path() / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(module_name.unique_name());
    const auto module_version = version_t(module_directory);
    auto module = std::make_unique<module_t>(workspace, module_name, module_version);
    return workspace->add_module(std::move(module));
}

module_t* workspace_graph_t::discover_module(module_name_t module_name) {
    return discover_module_impl(module_name);
}

} // namespace m03gagbhsp2drqq3gkop8pzfrm_workspace_graph
