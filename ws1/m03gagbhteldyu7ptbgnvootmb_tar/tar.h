#ifndef M03GAGBHTELDYU7PTBGNVOOTMB_TAR_TAR_H
# define M03GAGBHTELDYU7PTBGNVOOTMB_TAR_TAR_H

# include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>

/**
 * @brief Creates and extracts directory archives through the configured host tar tool.
 *
 * Operations run synchronously and retain no argument references. Returned path
 * values do not manage file or directory lifetime. Calls must not overlap other
 * guarded child-process execution; see
 * m03gagbhsvr0m5w15urj0o291m_process::create_and_wait_checked() for the signal and
 * execution restrictions. Keep inputs unchanged and output paths exclusively
 * available during each call; existence checks do not reserve names.
 */
namespace m03gagbhteldyu7ptbgnvootmb_tar {

/**
 * @brief Archives a directory's contents relative to `.` into a new .tar file.
 *
 * dir must be an existing directory. install_tar_path must not exist and must
 * have the case-sensitive extension `.tar`; missing output parents are created.
 * Runs tar with `-C dir .`, so extracting produces the contents directly in the
 * destination, without an enclosing directory named after dir. When the archive
 * path is a lexical child of dir, passes an exclusion for that relative path.
 * Returns a path value equal to install_tar_path after checking for regular-file
 * output; does not remove the input directory.
 *
 * Throws std::runtime_error for invalid inputs, an existing destination, an
 * unavailable tool, a failed command, or missing/non-regular output. Filesystem
 * and process exceptions propagate. On command or output-check failure, attempts
 * to remove the archive; created parents remain. Cleanup can itself throw, leaving
 * the archive behind and replacing the original exception.
 *
 * Given source/root.txt and source/nested/value.txt, with the archive and
 * extraction destination absent:
 * @code{.cpp}
 * #include <m03gagbhteldyu7ptbgnvootmb_tar/tar.h>
 * #include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
 *
 * int main() {
 *     namespace filesystem = m03gagbhsnusi43zogoacgj2ez_filesystem;
 *     const auto archive_path = m03gagbhteldyu7ptbgnvootmb_tar::tar(
 *         filesystem::path_t("source"),
 *         filesystem::path_t("archives/source.tar")
 *     );
 *     const auto extracted_path = m03gagbhteldyu7ptbgnvootmb_tar::untar(
 *         archive_path,
 *         filesystem::path_t("extracted")
 *     );
 *     // Files are extracted/root.txt and extracted/nested/value.txt.
 *     // The source directory and archive also remain available.
 * }
 * @endcode
 */
m03gagbhsnusi43zogoacgj2ez_filesystem::path_t tar(
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& dir,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& install_tar_path
);

/**
 * @brief Extracts a .tar archive into a new or existing directory without transactional rollback.
 *
 * tar_path must be an existing readable regular file with the case-sensitive
 * extension `.tar`. install_dir may be absent or an existing directory; an
 * existing non-directory is rejected. Creates a missing destination and parents,
 * then invokes tar with `-xf tar_path -C install_dir`. Existing directories need
 * not be empty: archive entries may overwrite matching files according to the
 * host tool's extraction behavior. The archive is preserved; the returned path
 * equals install_dir. See tar() for a complete creation/extraction example.
 *
 * Throws std::runtime_error for invalid inputs, an unavailable tool, or a failed
 * command (including corrupt archives). Filesystem and process exceptions
 * propagate. If tool setup or extraction fails after creating install_dir,
 * attempts to remove that directory recursively; any newly created ancestors
 * remain. An existing install_dir is never removed by this cleanup and may retain
 * partial additions or overwritten files after failure. Inspect or restore an
 * existing destination before retrying when partial extraction matters.
 *
 * Directory creation happens before the extraction cleanup scope and is not
 * rolled back if creation itself fails. Cleanup errors can leave a new destination
 * behind and replace the original exception.
 */
m03gagbhsnusi43zogoacgj2ez_filesystem::path_t untar(
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& tar_path,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& install_dir
);

} // namespace m03gagbhteldyu7ptbgnvootmb_tar


#endif // M03GAGBHTELDYU7PTBGNVOOTMB_TAR_TAR_H
