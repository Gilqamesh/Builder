#ifndef M03GAGBHTH67IRF210VI3BYVHK_WGET_WGET_H
# define M03GAGBHTH67IRF210VI3BYVHK_WGET_WGET_H

# include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>

# include <string>

namespace m03gagbhth67irf210vi3byvhk_wget {

/**
 * @brief Downloads a URL to a new file without verifying its contents.
 *
 * Requires a nonempty url and a nonexistent install_path. Creates missing parent
 * directories and waits for the configured host wget executable to finish.
 * Arguments are borrowed only for the call; the returned path is a value equal
 * to install_path and does not manage the downloaded file's lifetime.
 *
 * Throws std::runtime_error for an empty URL, an existing destination, an
 * unavailable tool, a failed command, or missing output after the command.
 * Filesystem and process exceptions propagate. On command or output-check
 * failure, attempts to remove install_path; created parents remain. Cleanup can
 * itself throw, leaving output behind and replacing the original exception.
 *
 * Calls must not overlap other guarded child-process execution in this process;
 * see m03gagbhsvr0m5w15urj0o291m_process::create_and_wait_checked() for the signal
 * and execution restrictions. Keep the destination exclusively available for
 * this call; the existence check does not reserve it against other processes.
 *
 * For a download checked against an expected SHA-256, use
 * m03gagbht7wqhtdg9hwdpmfn5o_download::fetch() from
 * `<m03gagbht7wqhtdg9hwdpmfn5o_download/download.h>`.
 *
 * With downloads/payload.bin absent, pass the URL as the first argument:
 * @code{.cpp}
 * #include <m03gagbhth67irf210vi3byvhk_wget/wget.h>
 * #include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
 *
 * int main(int argc, char** argv) {
 *     if (argc != 2) {
 *         return 1;
 *     }
 *     const auto path = m03gagbhth67irf210vi3byvhk_wget::download(
 *         argv[1],
 *         m03gagbhsnusi43zogoacgj2ez_filesystem::path_t("downloads/payload.bin")
 *     );
 *     // Consume path; the file remains after path is destroyed.
 * }
 * @endcode
 */
m03gagbhsnusi43zogoacgj2ez_filesystem::path_t download(const std::string& url, const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& install_path);

} // namespace m03gagbhth67irf210vi3byvhk_wget

#endif // M03GAGBHTH67IRF210VI3BYVHK_WGET_WGET_H
