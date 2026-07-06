#ifndef M03GAGBHTH67IRF210VI3BYVHK_WGET_API_H
# define M03GAGBHTH67IRF210VI3BYVHK_WGET_API_H

# include <m03gagbhsnusi43zogoacgj2ez_filesystem>

# include <string>

namespace m03gagbhth67irf210vi3byvhk_wget {

/**
 * Downloads url to a new file and returns install_path.
 */
m03gagbhsnusi43zogoacgj2ez_filesystem::path_t download(
    const std::string& url,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& install_path
);

} // namespace m03gagbhth67irf210vi3byvhk_wget

#endif // M03GAGBHTH67IRF210VI3BYVHK_WGET_API_H
