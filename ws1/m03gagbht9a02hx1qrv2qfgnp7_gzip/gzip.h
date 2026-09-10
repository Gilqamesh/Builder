#ifndef M03GAGBHT9A02HX1QRV2QFGNP7_GZIP_GZIP_H
# define M03GAGBHT9A02HX1QRV2QFGNP7_GZIP_GZIP_H

# include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>

/**
 * @brief Compresses and decompresses files through the configured host gzip tool.
 *
 * Operations run synchronously and retain no argument references. Returned path
 * values do not manage file lifetime. Calls must not overlap other guarded
 * child-process execution; see
 * m03gagbhsvr0m5w15urj0o291m_process::create_and_wait_checked() for the signal and
 * execution restrictions. Keep inputs unchanged and destination/temporary paths
 * exclusively available during each call; existence checks do not reserve names.
 */
namespace m03gagbht9a02hx1qrv2qfgnp7_gzip {

/**
 * @brief Compresses a regular file into a new .gz file while preserving the input.
 *
 * file must be an existing readable regular file. install_gzip_path must not
 * exist and must have the case-sensitive extension `.gz`. Creates missing output
 * parents. The writable output parent must also have unused siblings named
 * install_gzip_path + ".input" and install_gzip_path + ".input.gz"; collisions
 * are rejected without removing those entries.
 *
 * Compresses a temporary copy with gzip's `-n` option, omitting the original name
 * and timestamp from the gzip header. On success, temporary files are gone and
 * the returned path equals install_gzip_path. On copy, tool, output-check, or
 * rename failure, attempts to remove both temporary files; created parents remain.
 * Cleanup can itself throw, leaving temporary files and replacing the original
 * exception.
 *
 * Throws std::runtime_error for invalid inputs, path collisions, an unavailable
 * tool, a failed command, or missing temporary output. Filesystem and process
 * exceptions propagate.
 *
 * Given an existing inputs/payload.txt and unused outputs and temporary siblings:
 * @code{.cpp}
 * #include <m03gagbht9a02hx1qrv2qfgnp7_gzip/gzip.h>
 * #include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
 *
 * int main() {
 *     namespace filesystem = m03gagbhsnusi43zogoacgj2ez_filesystem;
 *     const auto compressed_path = m03gagbht9a02hx1qrv2qfgnp7_gzip::gzip(
 *         filesystem::path_t("inputs/payload.txt"),
 *         filesystem::path_t("compressed/payload.txt.gz")
 *     );
 *     const auto restored_path = m03gagbht9a02hx1qrv2qfgnp7_gzip::ungzip(
 *         compressed_path,
 *         filesystem::path_t("restored/payload.txt")
 *     );
 *     // The input, compressed file, and restored file all remain available.
 * }
 * @endcode
 */
m03gagbhsnusi43zogoacgj2ez_filesystem::path_t gzip(
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& file,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& install_gzip_path
);

/**
 * @brief Decompresses a .gz file into a new file while preserving the compressed input.
 *
 * gzip_path must be an existing readable regular file with the case-sensitive
 * extension `.gz`. install_file must not exist; its extension is unrestricted.
 * Creates missing output parents. The writable output parent must also have
 * unused siblings named install_file + ".input.gz" and install_file + ".input";
 * collisions are rejected without removing those entries.
 *
 * On success, temporary files are gone and the returned path equals install_file.
 * On copy, tool, output-check, or rename failure, attempts to remove both temporary
 * files; created parents remain. Cleanup can itself throw, leaving temporary files
 * and replacing the original exception.
 *
 * Throws std::runtime_error for invalid inputs, path collisions, an unavailable
 * tool, a failed command (including corrupt gzip data), or missing temporary
 * output. Filesystem and process exceptions propagate. See gzip() for a complete
 * compression/decompression example.
 */
m03gagbhsnusi43zogoacgj2ez_filesystem::path_t ungzip(
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& gzip_path,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& install_file
);

} // namespace m03gagbht9a02hx1qrv2qfgnp7_gzip


#endif // M03GAGBHT9A02HX1QRV2QFGNP7_GZIP_GZIP_H
