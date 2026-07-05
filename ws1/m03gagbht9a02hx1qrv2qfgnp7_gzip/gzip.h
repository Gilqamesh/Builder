#ifndef M03GAGBHT9A02HX1QRV2QFGNP7_GZIP_GZIP_H
# define M03GAGBHT9A02HX1QRV2QFGNP7_GZIP_GZIP_H

# include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>

namespace m03gagbht9a02hx1qrv2qfgnp7_gzip {

/**
 * Compresses file into a new .gz file and returns install_gzip_path.
 */
m03gagbhsnusi43zogoacgj2ez_filesystem::path_t gzip(
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& file,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& install_gzip_path
);

/**
 * Decompresses gzip_path into a new file and returns install_file.
 */
m03gagbhsnusi43zogoacgj2ez_filesystem::path_t ungzip(
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& gzip_path,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& install_file
);

} // namespace m03gagbht9a02hx1qrv2qfgnp7_gzip


#endif // M03GAGBHT9A02HX1QRV2QFGNP7_GZIP_GZIP_H
