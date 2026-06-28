#ifndef M03GAGBHT7WQHTDG9HWDPMFN5O_DOWNLOAD_DOWNLOAD_H
# define M03GAGBHT7WQHTDG9HWDPMFN5O_DOWNLOAD_DOWNLOAD_H

# include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>

# include <string>

namespace download {

/**
 * Download URL and expected lowercase SHA-256 checksum.
 */
struct source_lock_t {
    std::string url;
    std::string sha256;
};

/**
 * Downloads source_lock.url to output_path, verifies sha256, and returns output_path.
 */
m03gagbhsnusi43zogoacgj2ez_filesystem::path_t fetch(const source_lock_t& source_lock, const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& output_path);

} // namespace download

#endif // M03GAGBHT7WQHTDG9HWDPMFN5O_DOWNLOAD_DOWNLOAD_H
