#ifndef M03GAGBHT3SVCX3IGN454LFUP3_CMAKE_CMAKE_H
# define M03GAGBHT3SVCX3IGN454LFUP3_CMAKE_CMAKE_H

# include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>

# include <cstddef>
# include <optional>
# include <string>
# include <utility>
# include <vector>

/**
 * @brief Runs CMake configuration, builds, and installation as checked child processes.
 *
 * Uses the CMake executable configured by this module's builder.cpp (currently
 * /usr/bin/cmake), which must remain available and executable at runtime. Calls
 * wait for completion, inherit standard streams and the environment, and retain
 * no references to their arguments. Process failures propagate from
 * m03gagbhsvr0m5w15urj0o291m_process::create_and_wait_checked(); its signal and
 * process-wide child-guard restrictions apply, so do not overlap guarded calls.
 * Filesystem failures also propagate. Failed commands may leave partial build
 * or install results; these operations do not roll them back.
 *
 * Given a project/ directory containing CMakeLists.txt with install rules:
 * @code{.cpp}
 * #include <m03gagbht3svcx3ign454lfup3_cmake/cmake.h>
 * #include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
 *
 * #include <exception>
 * #include <iostream>
 * #include <optional>
 *
 * int main() {
 *     namespace cmake = m03gagbht3svcx3ign454lfup3_cmake;
 *     namespace filesystem = m03gagbhsnusi43zogoacgj2ez_filesystem;
 *     try {
 *         const filesystem::path_t source_dir("project");
 *         const filesystem::path_t build_dir("build/project");
 *         const filesystem::path_t install_dir("install/project");
 *         cmake::configure(source_dir, build_dir, {
 *             { "CMAKE_INSTALL_PREFIX", install_dir.string() }
 *         });
 *         cmake::build(build_dir, std::nullopt); // Bare -j; use std::size_t(2) to limit jobs.
 *         cmake::install(build_dir); // Reached only after successful configure/build.
 *         return 0;
 *     } catch (const std::exception& exception) {
 *         std::cerr << exception.what() << '\n';
 *         return 1;
 *     }
 * }
 * @endcode
 */
namespace m03gagbht3svcx3ign454lfup3_cmake {

/**
 * @brief Configures a CMake source tree into build_dir with ordered cache definitions.
 *
 * source_dir must exist and contain a project CMake can configure. Creates
 * build_dir and missing parents if absent; an existing build tree is reused.
 * Each pair becomes one literal `-Dkey=value` argument in vector order, without
 * shell quoting or expansion. CMake interprets keys and values. Use an explicit
 * CMAKE_INSTALL_PREFIX when installation must target a caller-chosen directory.
 *
 * @throws std::runtime_error If source_dir or the configured tool is missing,
 * or checked process execution fails. Other filesystem/process exceptions propagate.
 */
void configure(
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& source_dir,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& build_dir,
    const std::vector<std::pair<std::string, std::string>>& define_key_values
);

/**
 * @brief Builds an existing configured CMake build tree and waits for success.
 *
 * Passes `--build build_dir` and `-jN` for a supplied n_jobs, without validating
 * the count locally; use a positive count. std::nullopt passes bare `-j`, leaving
 * parallelism to the native build tool rather than imposing a job limit.
 * Does not configure the tree or select a particular target or configuration.
 *
 * @throws std::runtime_error If build_dir or the configured tool is missing,
 * or checked process execution fails. Other filesystem/process exceptions propagate.
 */
void build(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& build_dir, std::optional<std::size_t> n_jobs);

/**
 * @brief Runs the install rules of an existing, already built CMake tree.
 *
 * Passes `--install build_dir`, using the install prefix configured in that tree.
 * Does not build missing artifacts first; installed files may be overwritten
 * according to the project's install rules.
 *
 * @throws std::runtime_error If build_dir or the configured tool is missing,
 * or checked process execution fails. Other filesystem/process exceptions propagate.
 */
void install(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& build_dir);

} // namespace m03gagbht3svcx3ign454lfup3_cmake


#endif // M03GAGBHT3SVCX3IGN454LFUP3_CMAKE_CMAKE_H
