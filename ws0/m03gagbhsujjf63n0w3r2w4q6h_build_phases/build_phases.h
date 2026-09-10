#ifndef M03GAGBHSUJJF63N0W3R2W4Q6H_BUILD_PHASES_BUILD_PHASES_H
# define M03GAGBHSUJJF63N0W3R2W4Q6H_BUILD_PHASES_BUILD_PHASES_H

# include <m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain/cxx_toolchain.h>
# include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
# include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>

# include <memory>
# include <optional>
# include <string>
# include <string_view>
# include <vector>

namespace m03gagbhsujjf63n0w3r2w4q6h_build_phases {

struct library_phase_t;

/**
 * @brief Stages and installs a module's inputs and outputs through its phase chain.
 *
 * A producer exports `extern "C" void phase__<name>(const <name>_phase_t*)` from
 * builder.cpp. The pointer is borrowed only for that call: synchronously stage,
 * install, or register all outputs before returning, and do not retain the phase
 * pointer or use it from detached work. Const operations can mutate phase state
 * and artifacts; the interface does not synchronize concurrent use.
 *
 * Phase chains borrow their module and its graph, which must outlive the chain.
 * Producer failures propagate to installation; normal phase execution handles
 * incomplete-artifact cleanup. Filesystem path values do not own artifact files.
 */
class phase_base_t {
public:
    /**
     * @brief Retains an input's root and relative path for compile/link helpers.
     *
     * Owns path values for an earlier install root or a staged build root, not
     * the files. The selected files must remain available while consumed.
     */
    class built_t {
    public:
        /**
         * @brief Borrows the retained rooted path for this built value's lifetime.
         */
        const m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t& rooted_path() const;

    private:
        friend phase_base_t;

        explicit built_t(const m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t& rooted_path);

        m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t m_rooted_path;
    };

    virtual ~phase_base_t() = default;

    /**
     * @brief Creates an owning source/interface/library/binary chain borrowing module.
     *
     * Does not execute phases. The overload without a target permits all binary
     * targets; the target overload selects one name, or all names when empty.
     */
    static std::unique_ptr<phase_base_t> make(
        m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module
    );
    static std::unique_ptr<phase_base_t> make(
        m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module,
        std::string_view target
    );

    /**
     * @brief Phase name, such as source, interface, library, or binary.
     */
    std::string_view name() const;

    /**
     * @brief Returns the current phase's scratch directory after artifact resolution.
     *
     * Throws std::runtime_error if the phase's artifact directory has not been
     * resolved. Source, interface, and library producers receive resolved roots;
     * binary producers use install_binary()'s per-target directories instead.
     */
    m03gagbhsnusi43zogoacgj2ez_filesystem::path_t build_dir() const;

    /**
     * @brief Selects a path from an earlier phase install_dir() for compile/link helpers.
     *
     * Does not copy the input. A path must be a strict descendant of a resolved
     * earlier install root; a rooted path must retain that exact root. Inputs
     * outside those roots throw std::runtime_error.
     */
    built_t build(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& path) const;
    built_t build(const m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t& rooted_path) const;

    /**
     * @brief Selects a path from the installed source phase for compile/link helpers.
     *
     * Ensures source installation first, then selects an existing strict
     * descendant. Use from a later phase; the source phase cannot select itself
     * as an earlier input. Absolute, escaping, root-only, and missing paths throw.
     */
    built_t source(std::string_view relative_path) const;

    /**
     * @brief Stages an existing external input as a symlink under build_dir().
     *
     * The destination must be a new strict descendant with an existing parent
     * directory. Keep external available until consumers finish; it is not copied.
     * Missing inputs, existing destinations, and link-creation failures throw.
     */
    built_t build(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& external, const m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t& as) const;

    /**
     * @brief Publishes a path into the current phase install_dir() under the same relative path.
     *
     * Copies from the current build root or a resolved earlier install root.
     * Rooted inputs must retain one of those exact roots. Existing destinations
     * are rejected and missing parent directories are created. Staging a file
     * here alone does not mark the phase complete or update latest.
     */
    void install(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& path) const;
    void install(const m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t& rooted_path) const;
    void install(const built_t& built) const;

    /**
     * @brief Executes or reuses a phase in this chain and returns its installed paths.
     *
     * phase_t must be source_phase_t, interface_phase_t, library_phase_t, or
     * binary_phase_t. Throws if that phase is not in the chain; producer, build,
     * and validation failures propagate. Returned installed_t values own their
     * root paths and may outlive the chain, but do not keep files on disk alive.
     */
    template <class phase_t>
    typename phase_t::installed_t install() const;

protected:
    phase_base_t(
        std::string_view name,
        m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module,
        std::unique_ptr<phase_base_t> previous_phase
    );

    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module() const;
    m03gagbhsnusi43zogoacgj2ez_filesystem::path_t install_dir() const;

    void install_as(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& path, const m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t& relative_path) const;
    m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t installed_relative_path(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& path) const;

private:
    m03gagbhsnusi43zogoacgj2ez_filesystem::path_t artifact_dir() const;
    void artifact_dir(m03gagbhsnusi43zogoacgj2ez_filesystem::path_t artifact_dir) const;
    bool has_artifact_dir() const;
    m03gagbhsnusi43zogoacgj2ez_filesystem::path_t builder_plugin() const;
    const phase_base_t* previous_phase() const;

    template <class phase_t>
    void run_phase(const phase_t& requested_phase) const;

    template <class phase_t>
    typename phase_t::installed_t install(const phase_t& requested_phase) const;
    m03gagbhsnusi43zogoacgj2ez_filesystem::path_t install_library_scc(const library_phase_t& requested_phase) const;

private:
    std::string_view m_name;
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& m_module;
    std::unique_ptr<phase_base_t> m_previous_phase;
    mutable std::optional<m03gagbhsnusi43zogoacgj2ez_filesystem::path_t> m_artifact_dir;
};

/**
 * @brief Publishes the selected source tree for later phases.
 */
struct source_phase_t : phase_base_t {
    /**
     * @brief Retains a source install-root path by value.
     *
     * Direct construction copies a path without verifying phase completion.
     * root() borrows this value's storage.
     */
    class installed_t {
    public:
        explicit installed_t(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& root);

        /**
         * @brief Source phase install root.
         */
        const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& root() const;

    private:
        m03gagbhsnusi43zogoacgj2ez_filesystem::path_t m_root;
    };

    source_phase_t(
        m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module,
        std::unique_ptr<phase_base_t> previous_phase
    );

    /**
     * @brief Filesystem root of the current module source tree.
     */
    m03gagbhsnusi43zogoacgj2ez_filesystem::path_t source_dir() const;

    /**
     * @brief Publishes every file under source_dir().
     */
    void install_source_tree() const;

    /**
     * @brief Publishes a generated or downloaded source path from build_dir().
     */
    void install_source(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& source) const;
};

/**
 * @brief Publishes public include files beneath the complete module-name prefix.
 */
struct interface_phase_t : phase_base_t {
    /**
     * @brief Retains an interface install-root path by value.
     *
     * Direct construction copies a path without verifying phase completion.
     * root() borrows this value's storage.
     */
    class installed_t {
    public:
        explicit installed_t(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& root);

        /**
         * @brief Interface phase install root.
         */
        const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& root() const;

    private:
        m03gagbhsnusi43zogoacgj2ez_filesystem::path_t m_root;
    };

    interface_phase_t(
        m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module,
        std::unique_ptr<phase_base_t> previous_phase
    );

    /**
     * @brief Copies an installed source path into build_dir() at relative_path and returns the copied path.
     */
    m03gagbhsnusi43zogoacgj2ez_filesystem::path_t build_interface_as(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& source, const m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t& relative_path) const;

    /**
     * @brief Publishes all .h and .hpp files from the source phase.
     */
    void install_headers_from_source() const;

    /**
     * @brief Publishes an include path under `<module_name>/<relative_path>`.
     */
    void install_interface(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& interface) const;
    void install_interface(const m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t& interface) const;
};

/**
 * @brief Builds and stages a module library with validation required before completion.
 *
 * Builder constructs and validates every member of a library SCC before marking
 * any member complete or publishing latest. Register validation during the
 * producer call; execution occurs after the SCC's libraries have been staged.
 *
 * A minimal builder.cpp for a module containing `api.cpp` and `test/public_api.cpp`:
 * @code{.cpp}
 * #include <m03gagbhsujjf63n0w3r2w4q6h_build_phases/build_phases.h>
 *
 * namespace phases = m03gagbhsujjf63n0w3r2w4q6h_build_phases;
 *
 * extern "C" void phase__library(const phases::library_phase_t* phase) {
 *     const auto source_file = phase->source("api.cpp");
 *     const auto library = phase->build_library({ source_file }, {});
 *     phase->install_library(library);
 *     phase->validate_library("public_api", { phase->source("test/public_api.cpp") });
 * } // phase is no longer available to the producer after this call.
 * @endcode
 * The test source supplies main() and includes the module's public header to
 * establish its library dependency. Builder automatically registers
 * `test/public_api.cpp` when present unless `public_api` was explicitly registered.
 */
struct library_phase_t : phase_base_t {
    /** @brief Stores a validation executable's name, rooted sources, defines, and arguments by value. */
    struct validation_t {
        std::string name;
        std::vector<phase_base_t::built_t> source_files;
        std::vector<m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::define_t> defines;
        std::vector<std::string> arguments;
    };

    /**
     * @brief Retains a library install-root path by value.
     *
     * Direct construction copies a path without verifying phase completion.
     * root() borrows this value's storage.
     */
    class installed_t {
    public:
        explicit installed_t(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& root);

        /**
         * @brief Library phase install root.
         */
        const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& root() const;

    private:
        m03gagbhsnusi43zogoacgj2ez_filesystem::path_t m_root;
    };

    library_phase_t(
        m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module,
        std::unique_ptr<phase_base_t> previous_phase
    );

    /**
     * @brief Builds the module library and returns its output path.
     *
     * Resolves include dependencies from the source set and installs the required
     * interfaces. The result is under build_dir(); call install_library() to
     * stage it for publication. Compile/link failures propagate from the C++
     * toolchain. This operation does not run registered validation.
     */
    m03gagbhsnusi43zogoacgj2ez_filesystem::path_t build_library(
        const std::vector<phase_base_t::built_t>& source_files,
        const std::vector<m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::define_t>& defines
    ) const;

    /**
     * @brief Publishes a library path from build_dir().
     */
    void install_library(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& library) const;
    void install_library(const phase_base_t::built_t& library) const;

    /**
     * @brief Registers a test executable that must pass before this library is marked complete.
     *
     * Copies all arguments; does not compile or run the test during this call.
     * name must be non-empty and contain no path separators, and source_files
     * must be non-empty; violations throw std::runtime_error. Choose a distinct
     * filename other than `.` or `..` for each validation output.
     * arguments excludes the executable name. The runner executes in its own
     * validation build directory and must exit with status 0; build failures,
     * nonzero exits, and signal termination prevent SCC completion.
     */
    void validate_library(
        std::string_view name,
        const std::vector<phase_base_t::built_t>& source_files,
        const std::vector<m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::define_t>& defines = {},
        const std::vector<std::string>& arguments = {}
    ) const;

    /**
     * @brief Borrows the registered validations in registration order.
     *
     * The vector belongs to this phase; later registration can invalidate its
     * element references and iterators.
     */
    const std::vector<validation_t>& validations() const;

private:
    mutable std::vector<validation_t> m_validations;
};

/**
 * @brief Builds selected executable targets and publishes their runtime artifacts.
 */
struct binary_phase_t : phase_base_t {
    /**
     * @brief Retains a binary install-root path and locates published targets beneath it.
     *
     * Direct construction copies a path without verifying phase completion.
     * root() borrows this value's storage.
     */
    class installed_t {
    public:
        explicit installed_t(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& root);

        /**
         * @brief Binary phase install root.
         */
        const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& root() const;

        /**
         * @brief Locates an existing regular executable file for a published target.
         *
         * Resolves the target's installed executable and requires a regular file.
         * Throws std::runtime_error for an invalid or unpublished target.
         */
        m03gagbhsnusi43zogoacgj2ez_filesystem::path_t target(std::string_view target) const;

    private:
        m03gagbhsnusi43zogoacgj2ez_filesystem::path_t m_root;
    };

    binary_phase_t(
        m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module,
        std::string_view target,
        std::unique_ptr<phase_base_t> previous_phase
    );

    /**
     * @brief Returns whether target matches the selection, or all targets were requested.
     *
     * Performs no target-name validation.
     */
    bool should_install_target(std::string_view target) const;

    /**
     * @brief Builds source_files and publishes the executable plus runtime artifacts.
     * Each runtime artifact is installed under its source basename.
     * Unselected targets are ignored. For a selected target, use a non-empty
     * filename other than `.` or `..`, with no path separators. Source inputs
     * establish dependencies; runtime artifacts must be existing files or
     * directories. Build, installation, and output-check failures propagate.
     */
    void install_binary(
        std::string_view target,
        const std::vector<phase_base_t::built_t>& source_files,
        const std::vector<m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::define_t>& defines = {},
        const std::vector<phase_base_t::built_t>& runtime_artifacts = {}) const;

private:
    std::string m_target;
};

/** @brief Carries the direct module and builder dependency names found by source scanning. */
struct discovered_module_dependencies_t {
    std::vector<m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t> module_dependencies;
    std::vector<m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t> builder_dependencies;
};

/**
 * @brief Scans a module's library source set and builder.cpp for direct dependencies.
 *
 * Requires builder.cpp to exist. Uses the dependency eligibility and scanning
 * rules owned by m03gn8rf3pe86v64vphnaam6rl_source_dependencies; discovery and
 * scan failures propagate. Referenced modules may be materialized in the graph.
 */
discovered_module_dependencies_t discover_module_dependencies(
    const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module
);

} // namespace m03gagbhsujjf63n0w3r2w4q6h_build_phases

#endif // M03GAGBHSUJJF63N0W3R2W4Q6H_BUILD_PHASES_BUILD_PHASES_H
