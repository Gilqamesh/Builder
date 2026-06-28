#ifndef M03GAGBHT6JA46UIKB1LTAN0X8_DOT_DOT_H
# define M03GAGBHT6JA46UIKB1LTAN0X8_DOT_DOT_H

# include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>

namespace dot {

/**
 * Renders a DOT file to a new SVG file and returns output_svg_path.
 */
m03gagbhsnusi43zogoacgj2ez_filesystem::path_t render_svg(
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& dot_path,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& output_svg_path
);

} // namespace dot

#endif // M03GAGBHT6JA46UIKB1LTAN0X8_DOT_DOT_H
