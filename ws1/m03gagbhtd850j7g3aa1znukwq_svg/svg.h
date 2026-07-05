#ifndef M03GAGBHTD850J7G3AA1ZNUKWQ_SVG_SVG_H
# define M03GAGBHTD850J7G3AA1ZNUKWQ_SVG_SVG_H

# include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>

namespace m03gagbhtd850j7g3aa1znukwq_svg {

/**
 * Renders an SVG file to a new PNG file and returns output_png_path.
 */
m03gagbhsnusi43zogoacgj2ez_filesystem::path_t render_png(
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& svg_path,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& output_png_path
);

} // namespace m03gagbhtd850j7g3aa1znukwq_svg

#endif // M03GAGBHTD850J7G3AA1ZNUKWQ_SVG_SVG_H
