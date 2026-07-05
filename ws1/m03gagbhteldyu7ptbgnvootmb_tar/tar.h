#ifndef M03GAGBHTELDYU7PTBGNVOOTMB_TAR_TAR_H
# define M03GAGBHTELDYU7PTBGNVOOTMB_TAR_TAR_H

# include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>

namespace m03gagbhteldyu7ptbgnvootmb_tar {

/**
 * Archives dir into a new .tar file and returns install_tar_path.
 */
m03gagbhsnusi43zogoacgj2ez_filesystem::path_t tar(
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& dir,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& install_tar_path
);

/**
 * Extracts tar_path into install_dir and returns install_dir.
 */
m03gagbhsnusi43zogoacgj2ez_filesystem::path_t untar(
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& tar_path,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& install_dir
);

} // namespace m03gagbhteldyu7ptbgnvootmb_tar


#endif // M03GAGBHTELDYU7PTBGNVOOTMB_TAR_TAR_H
