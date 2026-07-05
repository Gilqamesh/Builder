#include <m03gagbht6ja46uikb1ltan0x8_dot/dot.h>

#include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
#include <m03gagbhsvr0m5w15urj0o291m_process/process.h>

#include <format>
#include <stdexcept>

#ifndef M03GAGBHT6JA46UIKB1LTAN0X8_DOT_DOT_PATH
# error M03GAGBHT6JA46UIKB1LTAN0X8_DOT_DOT_PATH must be defined by the owning builder
#endif

namespace dot {

static m03gagbhsnusi43zogoacgj2ez_filesystem::path_t dot_path() {
    const auto result = m03gagbhsnusi43zogoacgj2ez_filesystem::path_t(M03GAGBHT6JA46UIKB1LTAN0X8_DOT_DOT_PATH);
    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(result) || !m03gagbhsnusi43zogoacgj2ez_filesystem::is_regular_file(result)) {
        throw std::runtime_error(std::format("dot::render_svg: host tool '{}' does not exist or is not a regular file", result));
    }

    return result;
}

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t render_svg(
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& dot_file,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& output_svg_path
) {
    if (output_svg_path.extension() != ".svg") {
        throw std::runtime_error(std::format("dot::render_svg: output path '{}' must have .svg extension", output_svg_path));
    }

    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(dot_file) || !m03gagbhsnusi43zogoacgj2ez_filesystem::is_regular_file(dot_file)) {
        throw std::runtime_error(std::format("dot::render_svg: DOT file '{}' does not exist or is not a regular file", dot_file));
    }

    if (m03gagbhsnusi43zogoacgj2ez_filesystem::exists(output_svg_path)) {
        throw std::runtime_error(std::format("dot::render_svg: output path '{}' already exists", output_svg_path));
    }

    const auto parent = output_svg_path.parent();
    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(parent)) {
        m03gagbhsnusi43zogoacgj2ez_filesystem::create_directories(parent);
    }

    try {
        m03gagbhsvr0m5w15urj0o291m_process::create_and_wait_checked(m03gagbhsvr0m5w15urj0o291m_process::command_t {
            .args = {
                dot_path(),
                "-Tsvg",
                dot_file,
                "-o",
                output_svg_path
            }
        });

        if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(output_svg_path)) {
            throw std::runtime_error(std::format("dot::render_svg: expected output '{}' to exist but it does not", output_svg_path));
        }
    } catch (...) {
        m03gagbhsnusi43zogoacgj2ez_filesystem::remove(output_svg_path);
        throw ;
    }

    return output_svg_path;
}

} // namespace dot
