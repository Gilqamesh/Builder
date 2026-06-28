#include <m03gagbhsujjf63n0w3r2w4q6h_build_phases/build_phases.h>
#include <m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain/cxx_toolchain.h>
#include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>

#include <m03gagbht3svcx3ign454lfup3_cmake/cmake.h>
#include <m03gagbht7wqhtdg9hwdpmfn5o_download/download.h>
#include <m03gagbht9a02hx1qrv2qfgnp7_gzip/gzip.h>
#include <m03gagbhteldyu7ptbgnvootmb_tar/tar.h>

#include <cstddef>
#include <format>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace raylib {

static constexpr auto RAYLIB_SOURCE_URL = "https://codeload.github.com/raysan5/raylib/tar.gz/refs/tags/5.5";
static constexpr auto RAYLIB_SOURCE_SHA256 = "aea98ecf5bc5c5e0b789a76de0083a21a70457050ea4cc2aec7566935f5e258e";

extern "C" void phase__source(const m03gagbhsujjf63n0w3r2w4q6h_build_phases::source_phase_t* phase) {
    phase->install_source_tree();

    const auto raylib_tar_gz = download::fetch(
        download::source_lock_t {
            .url = RAYLIB_SOURCE_URL,
            .sha256 = RAYLIB_SOURCE_SHA256
        },
        phase->build_dir() / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("raylib-5.5.tar.gz")
    );

    const auto raylib_tar = gzip::ungzip(
        raylib_tar_gz,
        phase->build_dir() / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("raylib-5.5.tar")
    );
    const auto raylib_extract_dir = tar::untar(
        raylib_tar,
        phase->build_dir() / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("upstream")
    );
    const auto raylib_source_dir = raylib_extract_dir / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("raylib-5.5");
    phase->install_source(raylib_source_dir);
}

extern "C" void phase__interface(const m03gagbhsujjf63n0w3r2w4q6h_build_phases::interface_phase_t* phase) {
    const auto sources = phase->install<m03gagbhsujjf63n0w3r2w4q6h_build_phases::source_phase_t>();
    const std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t> headers = {
        m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("raylib.h"),
        m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("raymath.h"),
        m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("rlgl.h")
    };

    for (const auto& header : headers) {
        const auto interface_header = phase->build_interface_as(
            sources.root() / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(std::format("upstream/raylib-5.5/src/{}", header.string())),
            header
        );
        phase->install_interface(interface_header);
    }
}

extern "C" void phase__library(const m03gagbhsujjf63n0w3r2w4q6h_build_phases::library_phase_t* phase) {
    const auto sources = phase->install<m03gagbhsujjf63n0w3r2w4q6h_build_phases::source_phase_t>();
    const auto raylib_source_dir = sources.root() / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("upstream/raylib-5.5");
    const auto library_build_dir = phase->build_dir();
    const auto cmake_build_dir = library_build_dir / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("m03gagbht3svcx3ign454lfup3_cmake");
    const auto shared_libs = phase->library_type() == m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::library_type_t::SHARED;

    cmake::configure(raylib_source_dir, cmake_build_dir, {
        { "BUILD_SHARED_LIBS", shared_libs ? "ON" : "OFF" },
        { "BUILD_EXAMPLES", "OFF" },
        { "BUILD_GAMES", "OFF" },
        { "CMAKE_INSTALL_PREFIX", library_build_dir.string() }
    });

    cmake::build(cmake_build_dir, std::nullopt);
    cmake::install(cmake_build_dir);

    bool installed_library = false;
    for (const auto& artifact : m03gagbhsnusi43zogoacgj2ez_filesystem::find(
        library_build_dir,
        !m03gagbhsnusi43zogoacgj2ez_filesystem::find_include_predicate_t::is_dir,
        m03gagbhsnusi43zogoacgj2ez_filesystem::find_descend_predicate_t([&](const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& dir, std::size_t) {
            return dir != cmake_build_dir;
        })
    )) {
        const auto artifact_path = artifact.path();
        const auto filename = artifact_path.filename();
        if (filename.starts_with("libraylib") && (filename.find(".so") != std::string::npos || artifact_path.extension() == ".a")) {
            phase->install_library(artifact_path);
            installed_library = true;
        }
    }
    if (!installed_library) {
        throw std::runtime_error(std::format("libraries/m03gagbht17w4tser1fescqxye_raylib: expected raylib library under '{}'", library_build_dir));
    }
}

extern "C" void phase__binary(const m03gagbhsujjf63n0w3r2w4q6h_build_phases::binary_phase_t*) {
}

} // namespace raylib
