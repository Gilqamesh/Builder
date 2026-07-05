#ifndef M03GAGBHTBUSAQIDRTW6LNUGR4_SHA256SUM_SHA256SUM_H
# define M03GAGBHTBUSAQIDRTW6LNUGR4_SHA256SUM_SHA256SUM_H

# include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>

# include <string>

namespace sha256sum {

/**
 * Verifies path has expected_sha256.
 */
void verify(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& path, const std::string& expected_sha256);

} // namespace sha256sum

#endif // M03GAGBHTBUSAQIDRTW6LNUGR4_SHA256SUM_SHA256SUM_H
