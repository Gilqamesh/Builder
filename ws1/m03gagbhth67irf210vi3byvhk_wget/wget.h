#ifndef M03GAGBHTH67IRF210VI3BYVHK_WGET_WGET_H
# define M03GAGBHTH67IRF210VI3BYVHK_WGET_WGET_H

# include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>

# include <string>

namespace wget {

/**
 * Downloads url to a new file and returns install_path.
 */
m03gagbhsnusi43zogoacgj2ez_filesystem::path_t download(
    const std::string& url,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& install_path
);

} // namespace wget

#endif // M03GAGBHTH67IRF210VI3BYVHK_WGET_WGET_H
