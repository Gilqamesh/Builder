#ifndef M03GAGBHTBUSAQIDRTW6LNUGR4_SHA256SUM_API_H
# define M03GAGBHTBUSAQIDRTW6LNUGR4_SHA256SUM_API_H

# include <m03gagbhsnusi43zogoacgj2ez_filesystem>

# include <string>

namespace m03gagbhtbusaqidrtw6lnugr4_sha256sum {

/**
 * Verifies path has expected_sha256.
 */
void verify(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& path, const std::string& expected_sha256);

} // namespace m03gagbhtbusaqidrtw6lnugr4_sha256sum

#endif // M03GAGBHTBUSAQIDRTW6LNUGR4_SHA256SUM_API_H
