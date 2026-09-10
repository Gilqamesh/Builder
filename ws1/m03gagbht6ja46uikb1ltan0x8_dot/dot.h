#ifndef M03GAGBHT6JA46UIKB1LTAN0X8_DOT_DOT_H
# define M03GAGBHT6JA46UIKB1LTAN0X8_DOT_DOT_H

# include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>

namespace m03gagbht6ja46uikb1ltan0x8_dot {

/**
 * @brief Renders a regular DOT input file to a new SVG file using Graphviz dot.
 *
 * dot_path must name an existing regular file containing valid DOT; its extension
 * is not checked. output_svg_path must not exist and must have the case-sensitive
 * `.svg` extension. Creates missing output parent directories. The Graphviz tool
 * configured by this module's builder.cpp (currently /usr/bin/dot) must remain
 * available and executable at runtime.
 *
 * Waits for checked process completion and verifies that the output exists.
 * Borrows both paths only for the call and returns a copy of output_svg_path;
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
 * Given an existing graph.dot containing `digraph G { a -> b; }` and no destination:
 * @code{.cpp}
 * #include <m03gagbht6ja46uikb1ltan0x8_dot/dot.h>
 * #include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
 *
 * #include <exception>
 * #include <iostream>
 *
 * int main() {
 *     namespace filesystem = m03gagbhsnusi43zogoacgj2ez_filesystem;
 *     try {
 *         const filesystem::path_t dot_path("graph.dot");
 *         const filesystem::path_t output_svg_path("rendered/graph.svg");
 *         const auto rendered_path = m03gagbht6ja46uikb1ltan0x8_dot::render_svg(dot_path, output_svg_path);
 *         std::cout << rendered_path.string() << '\n';
 *         return 0;
 *     } catch (const std::exception& exception) {
 *         std::cerr << exception.what() << '\n';
 *         return 1;
 *     }
 * }
 * @endcode
 */
m03gagbhsnusi43zogoacgj2ez_filesystem::path_t render_svg(
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& dot_path,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& output_svg_path
);

} // namespace m03gagbht6ja46uikb1ltan0x8_dot

#endif // M03GAGBHT6JA46UIKB1LTAN0X8_DOT_DOT_H
