#ifndef M03GAGBHSMHR0NAW0ZPCCV4GAQ_CXX_TOOLCHAIN_CXX_TOOLCHAIN_H
# define M03GAGBHSMHR0NAW0ZPCCV4GAQ_CXX_TOOLCHAIN_CXX_TOOLCHAIN_H

# include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>

# include <string>
# include <vector>

namespace m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain {

/**
 * @brief Supplies library paths to the linker in caller-selected order.
 *
 * Paths must exist when linking. They follow the compiled objects without sorting
 * or deduplication; each parent directory is also added as a runtime library path.
 */
struct link_inputs_t {
    std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::path_t> libraries;
};

/**
 * @brief Owns a preprocessor definition whose replacement is a quoted string literal.
 *
 * The constructor copies or moves its arguments and rejects keys outside
 * `[A-Za-z_][A-Za-z0-9_]*` with std::runtime_error. Pass value without surrounding
 * quotes: build_library() and build_binary() add quotes and escape embedded
 * double quotes and backslashes. A value of `42` therefore defines a string,
 * not an integer token. Value contents are not validated by the constructor.
 */
class define_t {
public:
    define_t(std::string key, std::string value);

    /** @brief Borrows the stored identifier until this definition is modified or destroyed. */
    const std::string& key() const;
    /** @brief Borrows the unquoted text until this definition is modified or destroyed. */
    const std::string& value() const;

private:
    std::string m_key;
    std::string m_value;
};

/**
 * @brief Compiles rooted sources and links a shared library at output_path.
 *
 * Each source must still exist when compiled. Its relative path selects its
 * object path beneath build_dir, with the extension replaced by `.o`; choose
 * sources with distinct resulting object paths, even when their roots differ.
 * `.c` files use the configured C compiler; other extensions use the configured
 * C++26/reflection compiler. Include roots are passed as `-I` arguments in order.
 *
 * Creates build_dir, object parent directories, and output_path's parent as
 * needed. Existing objects/output may be overwritten. Calls are synchronous;
 * inputs are borrowed only for the call, and callers must coordinate builds
 * that write overlapping paths. The returned path is a copy of output_path.
 *
 * Missing inputs/tools, filesystem failures, and absent final output throw
 * std::runtime_error. Compiler/linker launch and checked-wait failures propagate
 * from m03gagbhsvr0m5w15urj0o291m_process::create_and_wait_checked(); a nonzero
 * exit or signal termination fails the build. Tool diagnostics use inherited
 * output streams. Failed builds can leave objects or output behind; this helper
 * does not roll back the build directory.
 *
 * Given existing `src/widget.cpp`, `include/`, and dependency libraries beneath
 * project_root (which must remain available throughout the call):
 * @code{.cpp}
 * #include <m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain/cxx_toolchain.h>
 * #include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
 *
 * namespace toolchain = m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain;
 * namespace filesystem = m03gagbhsnusi43zogoacgj2ez_filesystem;
 *
 * filesystem::path_t build_widget(const filesystem::path_t& project_root) {
 *     const filesystem::rooted_path_t source_file(
 *         project_root, filesystem::relative_path_t("src/widget.cpp"));
 *     const toolchain::link_inputs_t link_inputs {
 *         .libraries = {
 *             project_root / filesystem::relative_path_t("deps/libconsumer.so"),
 *             project_root / filesystem::relative_path_t("deps/libprovider.so")
 *         }
 *     };
 *     return toolchain::build_library(
 *         project_root / filesystem::relative_path_t("build/widget"),
 *         { project_root / filesystem::relative_path_t("include") },
 *         { source_file },
 *         { toolchain::define_t("MESSAGE", "hello \"builder\" \\ path") },
 *         link_inputs,
 *         project_root / filesystem::relative_path_t("output/libwidget.so")
 *     );
 * }
 * @endcode
 */
m03gagbhsnusi43zogoacgj2ez_filesystem::path_t build_library(
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& build_dir,
    const std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::path_t>& include_dirs,
    const std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t>& source_files,
    const std::vector<define_t>& defines,
    const link_inputs_t& link_inputs,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& output_path
);

/**
 * @brief Compiles rooted sources and links an executable at output_path.
 *
 * Uses the input, ordering, directory creation, lifetime, and failure contracts
 * of build_library(). Returns a copy of output_path after the checked link and
 * output-existence check; it does not run the executable.
 */
m03gagbhsnusi43zogoacgj2ez_filesystem::path_t build_binary(
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& build_dir,
    const std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::path_t>& include_dirs,
    const std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t>& source_files,
    const std::vector<define_t>& defines,
    const link_inputs_t& link_inputs,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& output_path
);

} // namespace m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain

#endif // M03GAGBHSMHR0NAW0ZPCCV4GAQ_CXX_TOOLCHAIN_CXX_TOOLCHAIN_H
