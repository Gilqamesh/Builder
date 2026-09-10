#ifndef M03GAGBHTBUSAQIDRTW6LNUGR4_SHA256SUM_SHA256SUM_H
# define M03GAGBHTBUSAQIDRTW6LNUGR4_SHA256SUM_SHA256SUM_H

# include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>

# include <string>

namespace m03gagbhtbusaqidrtw6lnugr4_sha256sum {

/**
 * @brief Checks a regular file against an expected SHA-256 using the host sha256sum tool.
 *
 * expected_sha256 must contain exactly 64 lowercase hexadecimal characters
 * (`0`-`9`, `a`-`f`), without a prefix or whitespace. path must name an existing
 * readable regular file. The input is not modified; arguments are borrowed only
 * for this synchronous call. Returning normally reports a checksum match.
 *
 * Current implementation limitation: newline-containing path names are not
 * escaped in the checksum file, so verification is unreliable for those names.
 *
 * Creates the temporary sibling path + ".sha256", so the parent must be writable
 * and that sibling must not exist. Removes the sibling after verification and
 * attempts to remove it on failure. A preexisting sibling is rejected and left
 * untouched. Cleanup can itself throw, leaving the sibling behind and replacing
 * the original exception. Keep the input unchanged and the sibling exclusively
 * available until the call finishes; its name is not atomically reserved.
 *
 * Throws std::runtime_error for an invalid checksum, invalid input, a sibling
 * collision, checksum-file write failure, an unavailable tool, or a nonzero tool
 * exit (including a checksum mismatch). Filesystem and process exceptions
 * propagate. Calls must not overlap other guarded child-process execution;
 * see m03gagbhsvr0m5w15urj0o291m_process::create_and_wait_checked() for the signal
 * and execution restrictions.
 *
 * Given inputs/abc.bin containing exactly the three bytes `abc`, with no
 * inputs/abc.bin.sha256 sibling:
 * @code{.cpp}
 * #include <m03gagbhtbusaqidrtw6lnugr4_sha256sum/sha256sum.h>
 * #include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
 *
 * int main() {
 *     const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t path("inputs/abc.bin");
 *     m03gagbhtbusaqidrtw6lnugr4_sha256sum::verify(
 *         path,
 *         "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"
 *     );
 *     // The original file remains; the temporary checksum sibling is gone.
 * }
 * @endcode
 */
void verify(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& path, const std::string& expected_sha256);

} // namespace m03gagbhtbusaqidrtw6lnugr4_sha256sum

#endif // M03GAGBHTBUSAQIDRTW6LNUGR4_SHA256SUM_SHA256SUM_H
