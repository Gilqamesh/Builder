#ifndef M03GAGBHSP2DRQQ3GKOP8PZFRM_WORKSPACE_GRAPH_WORKSPACE_GRAPH_H
# define M03GAGBHSP2DRQQ3GKOP8PZFRM_WORKSPACE_GRAPH_WORKSPACE_GRAPH_H

# include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
# include <m03gagbhtft23yhjwpp881tfmc_uuid/uuid.h>

# include <cstdint>
# include <functional>
# include <map>
# include <memory>
# include <set>
# include <string>
# include <string_view>
# include <utility>
# include <vector>

namespace m03gagbhsp2drqq3gkop8pzfrm_workspace_graph {

inline const constexpr char* BUILDER_CPP = "builder.cpp";
inline const constexpr char* CLI_CPP = "cli.cpp";

/**
 * @brief Stores a source version derived from file-clock timestamps or an explicit number.
 */
struct version_t {
    /**
     * @brief Uses value directly.
     */
    explicit version_t(uint64_t value);

    /**
     * @brief Converts a file timestamp to a version number.
     *
     * Preserves timestamp ordering by offsetting file-clock ticks into an unsigned
     * range. This is not a Unix timestamp; its units follow the file clock.
     */
    explicit version_t(const std::filesystem::file_time_type& file_time_type);

    /**
     * @brief Uses the latest timestamp of directory and entries visited by filesystem find().
     *
     * Includes the directory itself; directory symlinks below it are not traversed.
     * Filesystem query failures propagate.
     */
    explicit version_t(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& directory);

    uint64_t value;
};

/**
 * @brief Carries owning workspace and artifact path values for an invocation.
 */
struct invocation_context_t {
    m03gagbhsnusi43zogoacgj2ez_filesystem::path_t workspace_root;
    m03gagbhsnusi43zogoacgj2ez_filesystem::path_t artifact_root;
};

/**
 * @brief Validates and stores a complete UUIDv7-based module identity.
 */
class module_name_t {
public:
    /**
     * @brief Copies and validates a complete module name.
     *
     * Requires `m<25-character-base36-converted-uuidv7>_<friendly_name>`, where
     * friendly_name is non-empty and contains only alphanumeric characters and
     * underscores. Invalid spelling or UUID encoding throws; no filesystem
     * lookup is performed.
     */
    explicit module_name_t(std::string_view unique_name);

    /**
     * @brief Creates a module name from a friendly name, generating a new UUIDv7.
     */
    static module_name_t from_friendly_name(std::string_view friendly_name);

    /** @brief Borrows the complete name until this name object is modified or destroyed. */
    const std::string& unique_name() const;
    std::string friendly_name() const;
    m03gagbhtft23yhjwpp881tfmc_uuid::uuid uuid() const;

    bool operator==(const module_name_t& other) const;
    bool operator<(const module_name_t& other) const;
    bool operator<=(const module_name_t& other) const;

private:
    struct validated_name_t {
        std::string name;
    };

private:
    explicit module_name_t(validated_name_t validated_name) noexcept;
    std::array<std::byte, 16> base36_uuidv7_bytes(std::string_view view) const;

private:
    static constexpr std::size_t m_pos = 0;
    static constexpr std::size_t base36_decoded_uuidv7_start = m_pos + 1;
    static constexpr std::size_t base36_converted_uuidv7_size = 25;
    static constexpr std::size_t underscore_pos = base36_converted_uuidv7_size + 1;
    static constexpr std::size_t first_friendly_name_char_pos = underscore_pos + 1;

    std::string m_unique_name;
};

/**
 * @brief Hashes module_name_t by its string value.
 */
struct module_name_hash_t {
    std::size_t operator()(const module_name_t& module_name) const noexcept {
        return std::hash<std::string>()(module_name.unique_name());
    }
};

class workspace_t;

/**
 * @brief Associates a module identity and mutable source version with its containing workspace.
 *
 * Borrows its workspace, which must remain alive at the same address. Name and
 * workspace accessors return borrows; path accessors return owning values.
 */
class module_t {
public:
    /**
     * @brief Stores a name and version while borrowing a non-null workspace.
     *
     * Throws std::invalid_argument for a null workspace. Direct construction
     * does not register the module or check that its source directory exists.
     */
    module_t(const workspace_t* workspace, module_name_t name, version_t version);

    /**
     * @brief Workspace containing this module.
     */
    const workspace_t& workspace() const;

    /**
     * @brief Module name.
     */
    const module_name_t& name() const;

    /**
     * @brief Returns the stored source version without rescanning the source tree.
     */
    version_t version() const;

    /**
     * @brief Replaces the stored source version without changing files or artifacts.
     */
    void version(version_t version);

    /**
     * @brief Returns the source directory `<workspace_root>/<workspace>/<module>`.
     */
    m03gagbhsnusi43zogoacgj2ez_filesystem::path_t source_dir() const;

    /**
     * @brief Returns the artifact base directory `<artifact_root>/<module>`.
     */
    m03gagbhsnusi43zogoacgj2ez_filesystem::path_t artifact_base_dir() const;

    /**
     * @brief Returns the latest-artifact directory `<artifact_root>/<module>/latest`.
     */
    m03gagbhsnusi43zogoacgj2ez_filesystem::path_t artifact_latest_dir() const;

private:
    const workspace_t* m_workspace;
    version_t m_version;
    module_name_t m_name;
};

class workspace_graph_t;

/**
 * @brief Stores a `ws<order-position>` name and its numeric ordering position.
 */
class workspace_name_t {
public:
    /**
     * @brief Copies a workspace name with a non-empty decimal suffix fitting uint32_t.
     *
     * Invalid prefixes throw std::runtime_error; invalid or overflowing numeric
     * suffixes throw std::invalid_argument. Ordering compares numeric positions.
     */
    explicit workspace_name_t(std::string_view name);

    const m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t& relative_path() const;
    uint32_t order_position() const;

    bool operator==(const workspace_name_t& other) const;
    bool operator<(const workspace_name_t& other) const;
    bool operator<=(const workspace_name_t& other) const;

private:
    m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t m_relative_path;
    uint32_t m_order_position;
};

/**
 * @brief Hashes workspace_name_t by its string value.
 */
struct workspace_name_hash_t {
    std::size_t operator()(const workspace_name_t& workspace_name) const noexcept {
        return std::hash<m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t>()(workspace_name.relative_path());
    }
};

/**
 * @brief Owns materialized modules for one workspace while borrowing its graph.
 *
 * Keep the graph alive at the same address for this workspace's lifetime.
 * Returned module pointers borrow this workspace's storage. In a graph-created
 * workspace they therefore remain valid only while that graph is alive.
 */
class workspace_t {
public:
    /** @brief Creates an empty workspace without registering it in the graph. */
    workspace_t(workspace_graph_t& workspace_graph, workspace_name_t name);

    bool operator==(const workspace_t& other) const;
    bool operator<(const workspace_t& other) const;
    bool operator<=(const workspace_t& other) const;

    /**
     * @brief Graph containing this workspace.
     */
    workspace_graph_t& graph() const;

    /**
     * @brief Workspace name.
     */
    const workspace_name_t& name() const;
 
    /**
     * @brief Borrows an already materialized module by name, or returns nullptr.
     *
     * Does not consult the graph's discovery index or materialize a module.
     */
    module_t* find_module(const module_name_t& module_name) const;

    /**
     * @brief Takes ownership of a module and returns a borrowed pointer to it.
     *
     * The module must be non-null, belong to this workspace, and have a unique name.
     * Violations throw std::invalid_argument. Does not add a name to the graph's
     * discovery index; discover_module() remains limited to that index.
     */
    module_t* add_module(std::unique_ptr<module_t> module);

    /**
     * @brief Returns a snapshot of borrowed materialized modules sorted by complete name.
     */
    std::vector<module_t*> modules() const;

private:
    workspace_graph_t* m_workspace_graph;
    workspace_name_t m_name;
    std::map<module_name_t, std::unique_ptr<module_t>> m_module_by_name;
};

/**
 * @brief Indexes module locations and owns workspaces and lazily materialized modules.
 *
 * Returned workspace/module pointers and references borrow graph-owned objects;
 * keep the graph alive at the same address while using them. Further discovery
 * preserves existing object addresses. Returned vectors and name sets are
 * independent snapshots, not live views. Coordinate discovery or module mutation
 * with other access to the same graph; these operations have no internal locking.
 *
 * Enumerate the index first to materialize every module before reading modules():
 * @code{.cpp}
 * #include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>
 *
 * #include <iostream>
 *
 * namespace graph = m03gagbhsp2drqq3gkop8pzfrm_workspace_graph;
 *
 * int main() {
 *     const auto invocation_context = graph::invocation_context();
 *     graph::workspace_graph_t workspace_graph(
 *         invocation_context.workspace_root, invocation_context.artifact_root);
 *     for (const auto& module_name : workspace_graph.module_names()) {
 *         workspace_graph.discover_module(module_name);
 *     }
 *     for (const auto* module : workspace_graph.modules()) {
 *         std::cout << module->name().unique_name() << '\n';
 *     } // All borrowed pointers are used while workspace_graph is alive.
 * }
 * @endcode
 */
class workspace_graph_t {
public:
    /**
     * @brief Scans an existing workspace root and indexes direct module directories.
     *
     * Directory symlinks are accepted. Invalid workspace names are skipped;
     * invalid module names, duplicate module identities, duplicate workspace
     * ordering positions, and filesystem query failures throw. The artifact
     * root is stored without creating it. No module objects are materialized
     * until discover_module() or workspace_t::add_module() is called.
     */
    workspace_graph_t(m03gagbhsnusi43zogoacgj2ez_filesystem::path_t workspace_root, m03gagbhsnusi43zogoacgj2ez_filesystem::path_t artifact_dir);

    /**
     * @brief Borrows the stored workspace root for this graph's lifetime.
     */
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& root() const;

    /**
     * @brief Borrows the stored artifact root for this graph's lifetime.
     */
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& artifact_root() const;

    /**
     * @brief Materializes an indexed module and returns a non-null graph-owned borrow.
     *
     * Repeated calls return the existing object without refreshing its version.
     * First discovery derives the source version from the module directory.
     * Throws std::runtime_error if the name is absent; filesystem/version-query
     * failures propagate. Does not rescan the workspace's module index.
     */
    module_t* discover_module(module_name_t module_name);

    /**
     * @brief Returns borrowed workspaces sorted by numeric workspace position.
     */
    std::vector<const workspace_t*> workspaces() const;

    /**
     * @brief Returns borrowed materialized modules sorted by workspace position and complete name.
     *
     * Initially empty; indexed but unmaterialized names appear only in module_names().
     */
    std::vector<const module_t*> modules() const;

    /**
     * @brief Copies the construction-time discovery index as a set of complete names.
     *
     * Includes unmaterialized modules, ordered lexically by complete name rather
     * than workspace position. Call discover_module() to obtain their objects.
     */
    std::set<module_name_t> module_names() const;

private:
    module_t* discover_module_impl(module_name_t module_name);

private:
    std::map<workspace_name_t, std::unique_ptr<workspace_t>> m_workspace_by_workspace_name;
    std::map<module_name_t, workspace_t*> m_workspace_by_module_name;
    m03gagbhsnusi43zogoacgj2ez_filesystem::path_t m_root;
    m03gagbhsnusi43zogoacgj2ez_filesystem::path_t m_artifact_root;
};

/**
 * @brief Reads BUILDER_WORKSPACE_ROOT and BUILDER_ARTIFACT_ROOT, applies defaults, and exports the selected values.
 *
 * An unset workspace root defaults to current_path(); an unset artifact root
 * defaults to `<workspace_root>/artifacts`. Empty environment values throw
 * std::runtime_error. Relative values use path_t's current-directory resolution.
 * Both selected absolute paths are written to the process environment; calls
 * must be coordinated with other environment mutation. No directories are created.
 */
invocation_context_t invocation_context();

} // namespace m03gagbhsp2drqq3gkop8pzfrm_workspace_graph

namespace std {

template <>
struct formatter<m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();

        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid module_name_t format specifier");
        }

        return it;
    }

    auto format(const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t& module_name, auto& ctx) const {
        auto out = ctx.out();

        out = std::format_to(out, "{}", module_name.unique_name());

        return out;
    }
};

template <>
struct formatter<m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_name_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();

        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid workspace_name_t format specifier");
        }

        return it;
    }

    auto format(const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_name_t& workspace_name, auto& ctx) const {
        auto out = ctx.out();

        out = std::format_to(out, "{}", workspace_name.relative_path());

        return out;
    }
};

template <>
struct formatter<m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();

        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid module_t format specifier");
        }

        return it;
    }

    auto format(const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module, auto& ctx) const {
        auto out = ctx.out();

        out = std::format_to(out, "{}", module.name());

        return out;
    }
};

template <>
struct formatter<m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_t> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();

        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid workspace_t format specifier");
        }

        return it;
    }

    auto format(const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_t& workspace, auto& ctx) const {
        auto out = ctx.out();

        out = std::format_to(out, "{}", workspace.name());

        return out;
    }
};

} // namespace std

#endif // M03GAGBHSP2DRQQ3GKOP8PZFRM_WORKSPACE_GRAPH_WORKSPACE_GRAPH_H
