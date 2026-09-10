#ifndef M03GAGBHTD850J7G3AA1ZNUKWQ_SVG_SVG_H
# define M03GAGBHTD850J7G3AA1ZNUKWQ_SVG_SVG_H

# include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>

namespace m03gagbhtd850j7g3aa1znukwq_svg {

/**
 * @brief Renders a regular SVG input file to a new PNG file using rsvg-convert.
 *
 * svg_path must name an existing regular SVG file with the case-sensitive `.svg`
 * extension. output_png_path must not exist and must have the case-sensitive
 * `.png` extension. Creates missing output parent directories. The rsvg-convert
 * tool configured by this module's builder.cpp (currently /usr/bin/rsvg-convert)
 * must remain available and executable at runtime. Rendering uses the tool's
 * default size and resolution; this wrapper supplies no scaling options.
 *
 * Waits for checked process completion and verifies that the output exists.
 * Borrows both paths only for the call and returns a copy of output_png_path;
 * the caller manages the resulting file. The input is left unchanged. The
 * destination check does not reserve the path; serialize access to that output.
 * Execution inherits standard streams and follows the signal and process-wide
 * child-guard restrictions of
 * m03gagbhsvr0m5w15urj0o291m_process::create_and_wait_checked().
 *
 * If tool lookup, execution, or the output check fails, attempts to remove the
 * output before rethrowing. Created parent directories remain. Cleanup can itself
 * throw, leaving output behind and replacing the original exception.
 * @throws std::runtime_error For rejected paths, an unavailable configured tool,
 * failed checked execution, or missing output. Filesystem/process exceptions propagate.
 *
 * Given an existing image.svg and no destination:
 * @code{.cpp}
 * #include <m03gagbhtd850j7g3aa1znukwq_svg/svg.h>
 * #include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
 *
 * #include <exception>
 * #include <iostream>
 *
 * int main() {
 *     namespace filesystem = m03gagbhsnusi43zogoacgj2ez_filesystem;
 *     try {
 *         const filesystem::path_t svg_path("image.svg");
 *         const filesystem::path_t output_png_path("rendered/image.png");
 *         const auto rendered_path = m03gagbhtd850j7g3aa1znukwq_svg::render_png(svg_path, output_png_path);
 *         std::cout << rendered_path.string() << '\n';
 *         return 0;
 *     } catch (const std::exception& exception) {
 *         std::cerr << exception.what() << '\n';
 *         return 1;
 *     }
 * }
 * @endcode
 */
m03gagbhsnusi43zogoacgj2ez_filesystem::path_t render_png(
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& svg_path,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& output_png_path
);

} // namespace m03gagbhtd850j7g3aa1znukwq_svg

#endif // M03GAGBHTD850J7G3AA1ZNUKWQ_SVG_SVG_H
