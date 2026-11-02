#ifndef M03GAGBHTBUSAQIDRTW6LNUGR4_SHA256SUM_SHA256SUM_H
# define M03GAGBHTBUSAQIDRTW6LNUGR4_SHA256SUM_SHA256SUM_H

# include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>

# include <string>

namespace m03gagbhtbusaqidrtw6lnugr4_sha256sum {

/**
 * Verifies path has expected_sha256.
 */
void verify(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& path, const std::string& expected_sha256);

/**
 * Creates and returns path + ".sha256" for a regular file without changing it.
 * The destination must not exist. GNU checksum records use a relative filename,
 * so verification from their directory remains valid after moving both files.
 */
m03gagbhsnusi43zogoacgj2ez_filesystem::path_t generate(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& path);

} // namespace m03gagbhtbusaqidrtw6lnugr4_sha256sum

#endif // M03GAGBHTBUSAQIDRTW6LNUGR4_SHA256SUM_SHA256SUM_H
