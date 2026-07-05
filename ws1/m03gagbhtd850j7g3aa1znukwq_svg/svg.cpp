#include <m03gagbhtd850j7g3aa1znukwq_svg/svg.h>

#include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
#include <m03gagbhsvr0m5w15urj0o291m_process/process.h>

#include <format>
#include <stdexcept>
#include <string>

#ifndef M03GAGBHTD850J7G3AA1ZNUKWQ_SVG_RSVG_CONVERT_PATH
# error M03GAGBHTD850J7G3AA1ZNUKWQ_SVG_RSVG_CONVERT_PATH must be defined by the owning builder
#endif

namespace m03gagbhtd850j7g3aa1znukwq_svg {

static std::string rsvg_convert_string() {
    const auto result = m03gagbhsnusi43zogoacgj2ez_filesystem::path_t(M03GAGBHTD850J7G3AA1ZNUKWQ_SVG_RSVG_CONVERT_PATH);
    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(result) || !m03gagbhsnusi43zogoacgj2ez_filesystem::is_regular_file(result)) {
        throw std::runtime_error(std::format("svg::render_png: host tool '{}' does not exist or is not a regular file", result));
    }

    return result.string();
}

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t render_png(
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& svg_path,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& output_png_path
) {
    if (svg_path.extension() != ".svg") {
        throw std::runtime_error(std::format("svg::render_png: input path '{}' must have .svg extension", svg_path));
    }

    if (output_png_path.extension() != ".png") {
        throw std::runtime_error(std::format("svg::render_png: output path '{}' must have .png extension", output_png_path));
    }

    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(svg_path) || !m03gagbhsnusi43zogoacgj2ez_filesystem::is_regular_file(svg_path)) {
        throw std::runtime_error(std::format("svg::render_png: SVG file '{}' does not exist or is not a regular file", svg_path));
    }

    if (m03gagbhsnusi43zogoacgj2ez_filesystem::exists(output_png_path)) {
        throw std::runtime_error(std::format("svg::render_png: output path '{}' already exists", output_png_path));
    }

    const auto parent = output_png_path.parent();
    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(parent)) {
        m03gagbhsnusi43zogoacgj2ez_filesystem::create_directories(parent);
    }

    try {
        m03gagbhsvr0m5w15urj0o291m_process::create_and_wait_checked(m03gagbhsvr0m5w15urj0o291m_process::command_t({
            rsvg_convert_string(),
            svg_path.string(),
            "-o",
            output_png_path.string()
        }));

        if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(output_png_path)) {
            throw std::runtime_error(std::format("svg::render_png: expected output '{}' to exist but it does not", output_png_path));
        }
    } catch (...) {
        m03gagbhsnusi43zogoacgj2ez_filesystem::remove(output_png_path);
        throw ;
    }

    return output_png_path;
}

} // namespace m03gagbhtd850j7g3aa1znukwq_svg
