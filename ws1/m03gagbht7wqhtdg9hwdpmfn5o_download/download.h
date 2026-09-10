#ifndef M03GAGBHT7WQHTDG9HWDPMFN5O_DOWNLOAD_DOWNLOAD_H
# define M03GAGBHT7WQHTDG9HWDPMFN5O_DOWNLOAD_DOWNLOAD_H

# include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>

# include <string>

namespace m03gagbht7wqhtdg9hwdpmfn5o_download {

/**
 * @brief Pairs a download URL with the SHA-256 expected for its contents.
 *
 * Owns mutable strings; aggregate construction does not validate them. fetch()
 * requires a nonempty URL and the checksum format documented by
 * m03gagbhtbusaqidrtw6lnugr4_sha256sum::verify().
 */
struct source_lock_t {
    /// @brief URL passed to the downloader.
    std::string url;
    /// @brief Expected checksum as exactly 64 lowercase hexadecimal characters.
    std::string sha256;
};

/**
 * @brief Downloads a source to a new file and verifies its expected SHA-256.
 *
 * output_path must not exist; missing parents are created. source_lock.url must
 * be nonempty and source_lock.sha256 must contain exactly 64 characters from
 * `0`-`9` and `a`-`f`, without a prefix or whitespace. Empty fields are rejected
 * before download; other checksum-format errors are detected during verification
 * after downloading. Construction of source_lock_t establishes no validation.
 *
 * Uses m03gagbhth67irf210vi3byvhk_wget::download() followed by
 * m03gagbhtbusaqidrtw6lnugr4_sha256sum::verify(); their configured host tools,
 * failure modes, and process execution restrictions apply. Verification requires
 * a writable parent and an unused output_path + ".sha256" sibling; see its contract
 * in `<m03gagbhtbusaqidrtw6lnugr4_sha256sum/sha256sum.h>` for temporary-file rules
 * and current verification limitations.
 * Keep the destination and sibling exclusively available throughout the call.
 *
 * Throws std::runtime_error for empty fields and propagates download, verification,
 * filesystem, and process exceptions. On verification failure, attempts to remove
 * the downloaded file, including for a malformed checksum or sibling collision.
 * Created parents remain. Cleanup errors may leave files behind and replace the
 * original exception. An existing destination is rejected without downloading.
 *
 * Runs synchronously and retains no references to the arguments. Returns a path
 * value equal to output_path only after verification succeeds; the caller manages
 * the resulting file, whose lifetime is independent of the path and source lock.
 *
 * With downloads/abc.bin and its .sha256 sibling absent, pass a URL serving
 * exactly the three bytes `abc` as the first argument:
 * @code{.cpp}
 * #include <m03gagbht7wqhtdg9hwdpmfn5o_download/download.h>
 * #include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
 *
 * int main(int argc, char** argv) {
 *     if (argc != 2) {
 *         return 1;
 *     }
 *     const m03gagbht7wqhtdg9hwdpmfn5o_download::source_lock_t source_lock {
 *         .url = argv[1],
 *         .sha256 = "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"
 *     };
 *     const auto path = m03gagbht7wqhtdg9hwdpmfn5o_download::fetch(
 *         source_lock,
 *         m03gagbhsnusi43zogoacgj2ez_filesystem::path_t("downloads/abc.bin")
 *     );
 *     // Consume path only after fetch returns with verified contents.
 * }
 * @endcode
 */
m03gagbhsnusi43zogoacgj2ez_filesystem::path_t fetch(const source_lock_t& source_lock, const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& output_path);

} // namespace m03gagbht7wqhtdg9hwdpmfn5o_download

#endif // M03GAGBHT7WQHTDG9HWDPMFN5O_DOWNLOAD_DOWNLOAD_H
