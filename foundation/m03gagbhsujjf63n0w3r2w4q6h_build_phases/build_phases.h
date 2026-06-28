#ifndef M03GAGBHSUJJF63N0W3R2W4Q6H_BUILD_PHASES_H
# define M03GAGBHSUJJF63N0W3R2W4Q6H_BUILD_PHASES_H

# include <m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain/cxx_toolchain.h>
# include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
# include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>

# include <cstdint>
# include <memory>
# include <string>
# include <string_view>
# include <vector>

namespace m03gagbhsujjf63n0w3r2w4q6h_build_phases {

/**
 * Build phase identifier.
 */
enum class phase_id_t : uint8_t {
    SOURCE,
    INTERFACE,
    LIBRARY,
    BINARY
};

/**
 * Default phase order for a complete module build.
 */
inline std::vector<phase_id_t> default_phase_order() {
    return {
        phase_id_t::SOURCE,
        phase_id_t::INTERFACE,
        phase_id_t::LIBRARY,
        phase_id_t::BINARY
    };
}

/**
 * Library kind and phase order for a build.
 */
struct build_config_t {
    m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::library_type_t library_type;
    std::vector<phase_id_t> phase_order = default_phase_order();
};

/**
 * Common API available to every phase object.
 */
class phase_base_t {
public:
    /**
     * Compile/link input with its installed root preserved.
     */
    class built_t {
    public:
        /**
         * Input path and the install root it came from.
         */
        const m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t& rooted_path() const;

    private:
        friend phase_base_t;

        explicit built_t(const m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t& rooted_path);

        m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t m_rooted_path;
    };

    virtual ~phase_base_t() = default;

    /**
     * Creates the phase chain for module and build_config.
     */
    static std::unique_ptr<phase_base_t> make(
        m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module,
        build_config_t build_config
    );

    /**
     * Phase name, such as source, interface, library, or binary.
     */
    std::string_view name() const;

    /**
     * Private scratch directory for the current phase.
     */
    m03gagbhsnusi43zogoacgj2ez_filesystem::path_t build_dir() const;

    /**
     * Selects a path from an earlier phase install_dir() for compile/link helpers.
     */
    built_t build(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& path) const;
    built_t build(const m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t& rooted_path) const;

    /**
     * Publishes a path into the current phase install_dir() under the same relative path.
     */
    void install(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& path) const;
    void install(const m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t& rooted_path) const;

    /**
     * Installs phase_t for this module and returns its installed result.
     *
     * Example:
     * const auto sources = phase->install<source_phase_t>();
     */
    template <class phase_t>
    typename phase_t::installed_t install() const;

protected:
    phase_base_t(
        std::string_view name,
        m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module,
        build_config_t build_config,
        std::unique_ptr<phase_base_t> previous_phase
    );

    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module() const;
    build_config_t build_config() const;
    m03gagbhsnusi43zogoacgj2ez_filesystem::path_t install_dir() const;

    template <class phase_t>
    std::vector<typename phase_t::installed_t> install_closure() const;

    void install_as(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& path, const m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t& relative_path) const;
    m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t installed_relative_path(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& path) const;
    virtual void finalize_install() const;

private:
    m03gagbhsnusi43zogoacgj2ez_filesystem::path_t artifact_dir() const;
    m03gagbhsnusi43zogoacgj2ez_filesystem::path_t builder_plugin() const;
    const phase_base_t* previous_phase() const;

    template <class phase_t>
    typename phase_t::installed_t install(const phase_t& requested_phase) const;

private:
    std::string_view m_name;
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& m_module;
    build_config_t m_build_config;
    std::unique_ptr<phase_base_t> m_previous_phase;
};

/**
 * Phase object for publishing source files.
 */
struct source_phase_t : phase_base_t {
    class installed_t {
    public:
        explicit installed_t(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& root);

        /**
         * Source phase install root.
         */
        const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& root() const;

    private:
        m03gagbhsnusi43zogoacgj2ez_filesystem::path_t m_root;
    };

    source_phase_t(
        m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module,
        build_config_t build_config,
        std::unique_ptr<phase_base_t> previous_phase
    );

    /**
     * Filesystem root of the current module source tree.
     */
    m03gagbhsnusi43zogoacgj2ez_filesystem::path_t source_dir() const;

    /**
     * Publishes every file under source_dir().
     */
    void install_source_tree() const;

    /**
     * Publishes a generated or downloaded source path from build_dir().
     */
    void install_source(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& source) const;
};

/**
 * Phase object for publishing public include files.
 */
struct interface_phase_t : phase_base_t {
    class installed_t {
    public:
        explicit installed_t(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& root);

        /**
         * Interface phase install root.
         */
        const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& root() const;

    private:
        m03gagbhsnusi43zogoacgj2ez_filesystem::path_t m_root;
    };

    interface_phase_t(
        m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module,
        build_config_t build_config,
        std::unique_ptr<phase_base_t> previous_phase
    );

    /**
     * Copies an installed source path into build_dir() at relative_path and returns the copied path.
     */
    m03gagbhsnusi43zogoacgj2ez_filesystem::path_t build_interface_as(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& source, const m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t& relative_path) const;

    /**
     * Publishes all .h and .hpp files from the source phase.
     */
    void install_headers_from_source() const;

    /**
     * Publishes an include path under <module_name>/<relative path>.
     */
    void install_interface(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& interface) const;
    void install_interface(const m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t& interface) const;

    /**
     * Publishes an include path without the module-name prefix.
     */
    void install_interface_compatibility(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& interface) const;
    void install_interface_compatibility(const m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t& interface) const;
};

/**
 * Phase object for building and publishing the module library.
 */
struct library_phase_t : phase_base_t {
    class installed_t {
    public:
        explicit installed_t(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& root);

        /**
         * Library phase install root.
         */
        const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& root() const;

    private:
        m03gagbhsnusi43zogoacgj2ez_filesystem::path_t m_root;
    };

    library_phase_t(
        m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module,
        build_config_t build_config,
        std::unique_ptr<phase_base_t> previous_phase
    );

    /**
     * Selected library output kind for this build.
     */
    m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::library_type_t library_type() const;

    /**
     * Builds the module library and returns its output path.
     */
    m03gagbhsnusi43zogoacgj2ez_filesystem::path_t build_library(
        const std::vector<phase_base_t::built_t>& source_files,
        const std::vector<m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::define_t>& defines
    ) const;

    /**
     * Publishes a library path from build_dir().
     */
    void install_library(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& library) const;
};

/**
 * Phase object for building and publishing executable output.
 */
struct binary_phase_t : phase_base_t {
    class installed_t {
    public:
        explicit installed_t(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& root);

        /**
         * Binary phase install root.
         */
        const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& root() const;

        /**
         * Published default CLI path.
         */
        const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& cli() const;

    private:
        m03gagbhsnusi43zogoacgj2ez_filesystem::path_t m_root;
        m03gagbhsnusi43zogoacgj2ez_filesystem::path_t m_cli;
    };

    binary_phase_t(
        m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module,
        build_config_t build_config,
        std::unique_ptr<phase_base_t> previous_phase
    );

    /**
     * Builds the module's default CLI and returns its output path.
     */
    m03gagbhsnusi43zogoacgj2ez_filesystem::path_t build_cli(
        const std::vector<phase_base_t::built_t>& source_files,
        const std::vector<m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::define_t>& defines
    ) const;

    /**
     * Publishes binary as the default CLI path named cli.
     */
    void install_cli(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& binary) const;

    /**
     * Publishes a non-default binary under the same relative path.
     */
    void install_binary(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& binary) const;

protected:
    void finalize_install() const override;
};

} // namespace m03gagbhsujjf63n0w3r2w4q6h_build_phases

#endif // M03GAGBHSUJJF63N0W3R2W4Q6H_BUILD_PHASES_H
